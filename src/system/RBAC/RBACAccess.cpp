#include "pch.h"
#include "RBACAccess.h"

#include <span>
#include <string>

#include "ConnectionGuard.h"
#include "IMSDatabase.h"
#include "LoggerDefines.h"

AccessControl RBACAccess::_access{};
std::shared_mutex RBACAccess::_mutex{};
bool RBACAccess::_initialized = false;
bool RBACAccess::_loggedMissingInit = false;

void RBACAccess::Initialize(std::uint32_t accountId)
{
    std::unique_lock lock(_mutex);
    _initialized = true;
    _loggedMissingInit = false;
    _access.setAccount(accountId);

    _access.reload();
}

void RBACAccess::Shutdown()
{
    std::unique_lock lock(_mutex);
    _access.clear();
    _initialized = false;
    _loggedMissingInit = false;
}

void RBACAccess::SetCurrentAccount(std::uint32_t accountId)
{
    std::unique_lock lock(_mutex);
    if (!_initialized)
    {
        _initialized = true;
    }

    if (_access.getAccountId() == accountId && _access.isInitialized())
    {
        return;
    }

    _loggedMissingInit = false;
    _access.setAccount(accountId);
    _access.reload();
}

void RBACAccess::Reload()
{
    std::unique_lock lock(_mutex);
    if (!_initialized)
    {
        return;
    }

    _access.reload();
}

bool RBACAccess::HasPermission(std::uint32_t permId)
{
    std::shared_lock lock(_mutex);
    if (!IsReady())
    {
        return false;
    }

    return _access.hasPermission(permId);
}

bool RBACAccess::HasAny(std::initializer_list<std::uint32_t> ids)
{
    std::shared_lock lock(_mutex);
    if (!IsReady())
    {
        return false;
    }

    return _access.hasAny(std::span<const std::uint32_t>(ids.begin(), ids.size()));
}

bool RBACAccess::HasAll(std::initializer_list<std::uint32_t> ids)
{
    std::shared_lock lock(_mutex);
    if (!IsReady())
    {
        return false;
    }

    return _access.hasAll(std::span<const std::uint32_t>(ids.begin(), ids.size()));
}

void RBACAccess::Audit(std::uint32_t actorAccountId, const std::string& action)
{
    database::ConnectionGuardIMS guard(database::ConnectionType::Sync, false, "RBACAccess::Audit");
    if (!guard)
    {
        LOG_DEBUG("RBACAccess::Audit: No connection for actor {}", actorAccountId);
        return;
    }

    auto stmt = guard->GetPreparedStatement(database::Implementation::IMSPreparedStatement::DB_RBAC_AUDIT_INSERT);
    stmt->SetUInt(0, actorAccountId);
    stmt->SetString(1, action);

    if (!guard->ExecutePreparedInsert(*stmt))
    {
        LOG_DEBUG("RBACAccess::Audit: Insert failed for actor {}", actorAccountId);
    }
}

void RBACAccess::AuditGroupChange(std::uint32_t actorAccountId, std::uint32_t targetAccountId, std::uint32_t groupId)
{
    Audit(actorAccountId,
          "RBAC group update: account=" + std::to_string(targetAccountId) + " group=" + std::to_string(groupId));
}

void RBACAccess::AuditPermissionOverridePlaceholder(std::uint32_t actorAccountId, std::uint32_t targetAccountId,
                                                    std::uint32_t permissionId, bool granted)
{
    (void)actorAccountId;
    (void)targetAccountId;
    (void)permissionId;
    (void)granted;
    // TODO: Wire audit logging when permission override UI is added.
}

Permission RBACAccess::GetRBACRoleWithAccessRight(AccessRights rights)
{
    switch (rights)
    {
        case AccessRights::ACCESS_RIGHT_NONE:
            return Permission::ROLE_NONE;
        case AccessRights::ACCESS_RIGHT_NORMAL:
            return Permission::ROLE_OBSERVER;
        case AccessRights::ACCESS_RIGHT_NORMAL_PLUS:
            return Permission::ROLE_OPERATOR;
        case AccessRights::ACCESS_RIGHT_WAREHOUSEMAN:
            return Permission::ROLE_COORDINATOR;
        case AccessRights::ACCESS_RIGHT_USERMANAGER:
            return Permission::ROLE_MANAGER;
        case AccessRights::ACCESS_RIGHT_ADMIN:
            return Permission::ROLE_ADMIN;
        default:
            return Permission::ROLE_NONE;
    }
}

QString RBACAccess::DebugDump()
{
    std::shared_lock lock(_mutex);
    if (!_initialized)
    {
        return QString("RBACAccess not initialized");
    }

    return QString("RBACAccess account=%1 allowed=%2 denied=%3 granted=%4")
        .arg(_access.getAccountId())
        .arg(static_cast<qulonglong>(_access.allowedCount()))
        .arg(static_cast<qulonglong>(_access.deniedCount()))
        .arg(static_cast<qulonglong>(_access.grantedCount()));
}

bool RBACAccess::IsReady()
{
    if (!_initialized || !_access.isInitialized())
    {
        if (!_loggedMissingInit)
        {
            LOG_DEBUG("RBACAccess: Access control not initialized");
            _loggedMissingInit = true;
        }
        return false;
    }

    if (_access.getAccountId() == 0)
    {
        if (!_loggedMissingInit)
        {
            LOG_DEBUG("RBACAccess: No account context");
            _loggedMissingInit = true;
        }
        return false;
    }

    return true;
}
