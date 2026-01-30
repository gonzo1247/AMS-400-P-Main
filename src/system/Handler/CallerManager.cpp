#include "CallerManager.h"

#include "ConnectionGuard.h"
#include "GlobalSignals.h"
#include "pch.h"

bool CallerManager::LoadCallerData()
{
    // Implementation to load caller data from the database

    // SELECT id, department, phone, name, costUnit, location, is_active FROM caller_information LIMIT 100

    ConnectionGuardAMS connection(ConnectionType::Sync);

    auto select = connection->GetPreparedStatement(AMSPreparedStatement::DB_CI_SELECT_ALL_CALLERS);
    auto result = connection->ExecutePreparedSelect(*select);

    if (!result.IsValid())
        return false;

    uint32 counter = 0;
    callerMap.clear();

    while (result.Next())
    {
        Field* fields = result.Fetch();
        CallerInformation caller;
        caller.id = fields[0].GetUInt64();
        caller.department = fields[1].GetString();
        caller.phone = fields[2].GetString();
        caller.name = fields[3].GetString();
        caller.costUnitID = fields[4].GetUInt16();
        caller.companyLocationID = fields[5].GetUInt32();
        caller.is_active = fields[6].GetBool();

        callerMap[caller.id] = caller;

        counter++;
    }

    LOG_DEBUG("CallerManager::LoadCallerData: Loaded {} callers from database.", counter);

    return true;
}

bool CallerManager::AddCaller(const CallerInformation& caller)
{
    // Implementation to add a new caller to the database

    // INSERT INTO caller_information (department, phone, name, cost_unit_id, company_location_id, is_active) VALUES (?, ?, ?, ?, ?, ?)

    ConnectionGuardAMS connection(ConnectionType::Sync);

    auto insert = connection->GetPreparedStatement(AMSPreparedStatement::DB_CI_INSERT_NEW_CALLER);
    insert->SetString(0, caller.department);
    insert->SetString(1, caller.phone);
    insert->SetString(2, caller.name);
    insert->SetUInt(3, caller.costUnitID);
    insert->SetUInt(4, caller.companyLocationID);
    insert->SetBool(5, caller.is_active);

    if (connection->ExecutePreparedInsert(*insert))
    {
        LOG_DEBUG("CallerManager::AddCaller: Added caller '{}' to database.", caller.name);

        if (auto callerID = GetNewCallerID(caller))
            if (callerID != 0)
            {
                CallerInformation newCaller = caller;
                newCaller.id = callerID;
                callerMap[newCaller.id] = newCaller;
                emit GlobalSignals::instance()->SignalReloadCallerTable();
            }
        return true;
    }
    else
    {
        LOG_ERROR("CallerManager::AddCaller: Failed to add caller '{}' to database.", caller.name);
        return false;
    }
}

bool CallerManager::EditCaller(const CallerInformation& caller)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);

    auto updateStmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_CI_UPDATE_CALLER_BY_ID);
    updateStmt->SetString(0, caller.department);
    updateStmt->SetString(1, caller.phone);
    updateStmt->SetString(2, caller.name);
    updateStmt->SetUInt(3, caller.costUnitID);
    updateStmt->SetUInt(4, caller.companyLocationID);
    updateStmt->SetBool(5, caller.is_active);
    updateStmt->SetUInt64(6, caller.id);

    if (connection->ExecutePreparedInsert(*updateStmt))
    {
        LOG_DEBUG("CallerManager::EditCaller: Updated caller '{}' with ID '{}' in database.", caller.name, caller.id);
        auto data = callerMap.find(caller.id);
        if (data != callerMap.end())
        {
            data->second = caller;
        }
        emit GlobalSignals::instance()->SignalReloadCallerTable();
        return true;
    }
    else
    {
        LOG_ERROR("CallerManager::EditCaller: Failed to update caller '{}' in database.", caller.name);
        return false;
    }
}

bool CallerManager::RemoveCaller(std::uint64_t callerID)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    
    auto deleteStmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_CI_DELETE_CALLER_BY_ID);
    deleteStmt->SetUInt64(0, callerID);

    if (connection->ExecutePreparedInsert(*deleteStmt))
    {
        LOG_DEBUG("CallerManager::RemoveCaller: Caller with ID '{}' removed from database.", callerID);
        return true;
    }
    else
    {
        LOG_ERROR("CallerManager::RemoveCaller: Failed to remove caller with ID '{}'.", callerID);
        return false;
    }
}

std::unordered_map<std::uint64_t, CallerInformation> CallerManager::GetCallerMap() const
{
    return callerMap;
}

std::uint64_t CallerManager::GetNewCallerID(const CallerInformation& info)
{
    // SELECT id FROM caller_information WHERE phone = ? AND name = ? AND location = ? AND is_active = 1
    ConnectionGuardAMS connection(ConnectionType::Sync);

    auto select = connection->GetPreparedStatement(AMSPreparedStatement::DB_CI_SELECT_CALLER_AFTER_INSERT);
    select->SetString(0, info.phone);
    select->SetString(1, info.name);
    select->SetUInt(2, info.companyLocationID);


    auto result = connection->ExecutePreparedSelect(*select);

    if (result.IsValid() && result.Next())
    {
        Field* fields = result.Fetch();
        return fields[0].GetUInt64();
    }
    
    return 0;
}
