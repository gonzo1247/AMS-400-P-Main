#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <unordered_map>

#include "UserManagement.h"

class UserCache
{
   public:
    static UserCache& instance()
    {
        static UserCache cache;
        return cache;
    }

    std::optional<UserData> GetUserDataByID(std::uint32_t userID);

    void InvalidateUser(std::uint32_t userID);
    void InvalidateAll();

   private:
    UserCache() = default;
    ~UserCache() = default;

    UserCache(const UserCache&) = delete;
    UserCache& operator=(const UserCache&) = delete;

    std::optional<UserData> LoadUserFromDatabase(std::uint32_t userID);

   private:
    std::mutex _mutex;
    std::unordered_map<std::uint32_t, UserData> _cache;
};

// free function helper
inline std::optional<UserData> GetUserDataByID(std::uint32_t userID)
{
    return UserCache::instance().GetUserDataByID(userID);
}
