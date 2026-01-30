#pragma once

#include <QString>
#include <cstdint>
#include <initializer_list>
#include <shared_mutex>
#include <string>

#include "AccessControl.h"
#include "PermissionRBAC.h"
#include "SharedDefines.h"


class RBACAccess
{
   public:
    static void Initialize(std::uint32_t accountId);
    static void Shutdown();
    static void SetCurrentAccount(std::uint32_t accountId);
    static void Reload();

    static bool HasPermission(std::uint32_t permId);
    static bool HasAny(std::initializer_list<std::uint32_t> ids);
    static bool HasAll(std::initializer_list<std::uint32_t> ids);

    static void Audit(std::uint32_t actorAccountId, const std::string& action);
    static void AuditGroupChange(std::uint32_t actorAccountId, std::uint32_t targetAccountId, std::uint32_t groupId);
    static void AuditPermissionOverridePlaceholder(std::uint32_t actorAccountId, std::uint32_t targetAccountId,
                                                   std::uint32_t permissionId, bool granted);

    static Permission GetRBACRoleWithAccessRight(AccessRights rights);
    static QString DebugDump();

   private:
    static bool IsReady();

    static AccessControl _access;
    static std::shared_mutex _mutex;
    static bool _initialized;
    static bool _loggedMissingInit;
};
