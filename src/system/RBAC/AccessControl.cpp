#include "pch.h"
#include "AccessControl.h"

#include <algorithm>

#include "ConnectionGuard.h"
#include "DatabaseConnection.h"
#include "Field.h"
#include "IMSDatabase.h"
#include "LoggerDefines.h"

AccessControl::AccessControl() : _accountId(0), _allowed(), _denied(), _granted(), _linkedCache(), _initialized(false)
{
}

void AccessControl::setAccount(std::uint32_t accountId) { _accountId = accountId; }

void AccessControl::clear()
{
    _accountId = 0;
    _allowed.clear();
    _denied.clear();
    _granted.clear();
    _linkedCache.clear();
    _initialized = false;
}

bool AccessControl::hasPermission(std::uint32_t permId) const
{
    if (_denied.contains(permId))
    {
        return false;
    }

    if (_allowed.contains(permId))
    {
        return true;
    }

    if (_granted.contains(permId))
    {
        return true;
    }

    return false;
}

bool AccessControl::hasAny(std::span<const std::uint32_t> permIds) const
{
    for (const auto permId : permIds)
    {
        if (hasPermission(permId))
        {
            return true;
        }
    }

    return false;
}

bool AccessControl::hasAll(std::span<const std::uint32_t> permIds) const
{
    for (const auto permId : permIds)
    {
        if (!hasPermission(permId))
        {
            return false;
        }
    }

    return true;
}

void AccessControl::reload()
{
    _allowed.clear();
    _denied.clear();
    _granted.clear();
    _linkedCache.clear();
    _initialized = false;

    if (_accountId == 0)
    {
        return;
    }

    database::ConnectionGuardIMS guard(database::ConnectionType::Sync, false, "AccessControl::reload");
    if (!guard)
    {
        LOG_DEBUG("AccessControl::reload: No connection for account {}", _accountId);
        return;
    }

    auto groupStmt =
        guard->GetPreparedStatement(database::Implementation::IMSPreparedStatement::DB_RBAC_SELECT_ACCOUNT_GROUPS);
    groupStmt->SetUInt(0, _accountId);
    auto groupResult = guard->ExecutePreparedSelect(*groupStmt);

    std::unordered_set<std::uint32_t> expandedGroups;
    if (groupResult.IsValid())
    {
        while (groupResult.Next())
        {
            Field* fields = groupResult.Fetch();
            if (!fields || fields[0].IsNull())
            {
                continue;
            }

            const auto groupId = fields[0].GetUInt32();
            for (std::uint32_t id = 1; id <= groupId; ++id)
            {
                expandedGroups.insert(id);
            }
        }
    }

    std::unordered_set<std::uint32_t> basePermissions;
    for (const auto groupId : expandedGroups)
    {
        auto permStmt = guard->GetPreparedStatement(
            database::Implementation::IMSPreparedStatement::DB_RBAC_SELECT_GROUP_PERMISSIONS);
        permStmt->SetUInt(0, groupId);
        auto permResult = guard->ExecutePreparedSelect(*permStmt);
        if (!permResult.IsValid())
        {
            continue;
        }

        while (permResult.Next())
        {
            Field* fields = permResult.Fetch();
            if (!fields || fields[0].IsNull())
            {
                continue;
            }

            basePermissions.insert(fields[0].GetUInt32());
        }
    }

    std::unordered_set<std::uint32_t> expandedPermissions;
    std::unordered_set<std::uint32_t> visiting;
    bool cycleLogged = false;
    for (const auto permissionId : basePermissions)
    {
        expandPermission(permissionId, *guard, expandedPermissions, visiting, cycleLogged);
    }

    auto overridesStmt =
        guard->GetPreparedStatement(database::Implementation::IMSPreparedStatement::DB_RBAC_SELECT_ACCOUNT_OVERRIDES);
    overridesStmt->SetUInt(0, _accountId);
    auto overridesResult = guard->ExecutePreparedSelect(*overridesStmt);
    if (overridesResult.IsValid())
    {
        while (overridesResult.Next())
        {
            Field* fields = overridesResult.Fetch();
            if (!fields || fields[0].IsNull() || fields[1].IsNull())
            {
                continue;
            }

            const auto permissionId = fields[0].GetUInt32();
            const bool granted = fields[1].GetUInt8() > 0;
            if (granted)
            {
                _granted.insert(permissionId);
            }
            else
            {
                _denied.insert(permissionId);
            }
        }
    }

    _allowed = std::move(expandedPermissions);
    _initialized = true;

    LOG_DEBUG("AccessControl::reload: account={} groups={} basePerms={} expandedPerms={} denied={} granted={}",
              _accountId, expandedGroups.size(), basePermissions.size(), _allowed.size(), _denied.size(),
              _granted.size());
}

void AccessControl::expandPermission(std::uint32_t permId, database::DatabaseConnection& connection,
                                     std::unordered_set<std::uint32_t>& out,
                                     std::unordered_set<std::uint32_t>& visiting, bool& cycleLogged)
{
    if (visiting.contains(permId))
    {
        if (!cycleLogged)
        {
            LOG_DEBUG("AccessControl::expandPermission: Linked permission cycle detected");
            cycleLogged = true;
        }
        return;
    }

    if (!out.insert(permId).second)
    {
        return;
    }

    visiting.insert(permId);
    const auto& linkedPermissions = getLinkedPermissions(permId, connection);
    for (const auto linkedId : linkedPermissions)
    {
        expandPermission(linkedId, connection, out, visiting, cycleLogged);
    }
    visiting.erase(permId);
}

const std::vector<std::uint32_t>& AccessControl::getLinkedPermissions(std::uint32_t permId,
                                                                      database::DatabaseConnection& connection)
{
    const auto cached = _linkedCache.find(permId);
    if (cached != _linkedCache.end())
    {
        return cached->second;
    }

    auto& stored = _linkedCache[permId];
    auto stmt = connection.GetPreparedStatement(
        database::Implementation::IMSPreparedStatement::DB_RBAC_SELECT_LINKED_PERMISSION);
    stmt->SetUInt(0, permId);
    auto result = connection.ExecutePreparedSelect(*stmt);
    if (result.IsValid())
    {
        while (result.Next())
        {
            Field* fields = result.Fetch();
            if (!fields || fields[0].IsNull())
            {
                continue;
            }

            stored.push_back(fields[0].GetUInt32());
        }
    }

    return stored;
}
