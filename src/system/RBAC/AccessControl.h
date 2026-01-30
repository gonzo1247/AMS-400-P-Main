#pragma once

#include <cstdint>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace database
{
    class DatabaseConnection;
}

class AccessControl
{
   public:
    AccessControl();

    void setAccount(std::uint32_t accountId);
    void reload();
    void clear();

    bool hasPermission(std::uint32_t permId) const;
    bool hasAny(std::span<const std::uint32_t> permIds) const;
    bool hasAll(std::span<const std::uint32_t> permIds) const;

    bool isInitialized() const { return _initialized; }
    std::uint32_t getAccountId() const { return _accountId; }
    std::size_t allowedCount() const { return _allowed.size(); }
    std::size_t deniedCount() const { return _denied.size(); }
    std::size_t grantedCount() const { return _granted.size(); }

   private:
    void expandPermission(std::uint32_t permId, database::DatabaseConnection& connection,
                          std::unordered_set<std::uint32_t>& out, std::unordered_set<std::uint32_t>& visiting,
                          bool& cycleLogged);
    const std::vector<std::uint32_t>& getLinkedPermissions(std::uint32_t permId,
                                                           database::DatabaseConnection& connection);

    std::uint32_t _accountId;
    std::unordered_set<std::uint32_t> _allowed;
    std::unordered_set<std::uint32_t> _denied;
    std::unordered_set<std::uint32_t> _granted;
    std::unordered_map<std::uint32_t, std::vector<std::uint32_t>> _linkedCache;
    bool _initialized;
};
