#include "pch.h"

#include "UserCache.h"
#include "ConnectionGuard.h"
#include "DatabaseConnection.h"
#include "PreparedStatementRegistry.h"

std::optional<UserData> UserCache::GetUserDataByID(std::uint32_t userID)
{
    {
        std::scoped_lock lock(_mutex);

        auto it = _cache.find(userID);
        if (it != _cache.end())
        {
            return it->second;
        }
    }

    auto loaded = LoadUserFromDatabase(userID);
    if (!loaded.has_value())
    {
        return std::nullopt;
    }

    {
        std::scoped_lock lock(_mutex);
        _cache.emplace(userID, *loaded);
    }

    return loaded;
}

void UserCache::InvalidateUser(std::uint32_t userID)
{
    std::scoped_lock lock(_mutex);
    _cache.erase(userID);
}

void UserCache::InvalidateAll()
{
    std::scoped_lock lock(_mutex);
    _cache.clear();
}

std::optional<UserData> UserCache::LoadUserFromDatabase(std::uint32_t userID)
{
    try
    {
        //          0           1           2       3               4               5               6 7 8
        // SELECT u.Username, u.ChipID, u.Email, u.AccessRights, ud.FirstName, ud.LastName, ud.TextMailOrder,
        // ud.TextMailOfferRequest, ud.SubjectOfferRequestMail,
        //          9                   10                  11                  12
        // ud.SubjectOrderMail, ud.InternPhoneNumber, ud.PersonalStyle, ud.PersonalLanguage
        ConnectionGuardIMS connection(ConnectionType::Sync);

        auto stmt = connection->GetPreparedStatement(IMSPreparedStatement::DB_USER_SELECT_USER_BY_ID);
        stmt->SetUInt(0, userID);

        auto result = connection->ExecutePreparedSelect(*stmt);

        if (!result.IsValid() || !result.Next())
            return std::nullopt;

        Field* fields = result.Fetch();
        UserData data;
        data.userID = userID;
        data.userName = fields[0].GetString();
        data.chipID = fields[1].GetString();
        data.userEmail = fields[2].GetString();
        data.userAccessRights = static_cast<AccessRights>(fields[3].GetUInt8());
        data.userFirstName = fields[4].GetString();
        data.userLastName = fields[5].GetString();
        data.userPhoneNumber = fields[10].GetString();
        data.personalStyle = fields[11].GetInt32();

        if (!fields[12].IsNull())
            data.personalLanguage = fields[12].GetString();
        else
            data.personalLanguage = {};
        

        return data;
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR("UserCache::LoadUserFromDatabase: exception: {}", ex.what());
    }

    return std::nullopt;
}
