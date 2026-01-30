#include "EmployeeManager.h"

#include <ranges>

#include "ConnectionGuard.h"
#include "DatabaseTypes.h"
#include "GlobalSignals.h"
#include "UserManagement.h"
#include "pch.h"

void EmployeeManager::LoadEmployeeData()
{
    // Implementation to load employee data from the database

    // SELECT id, firstName, lastName, phone, location, isActive FROM employees

    ConnectionGuardAMS connection(ConnectionType::Sync);

    auto select = connection->GetPreparedStatement(AMSPreparedStatement::DB_EI_SELECT_ALL_EMPLOYEES);
    auto result = connection->ExecutePreparedSelect(*select);

    if (!result.IsValid())
        return;

    int count = 0;
    employeeMap.clear();

    while (result.Next())
    {        
        EmployeeInformation employee;

        Field* fields = result.Fetch();

        employee.id = fields[0].GetUInt32();
        employee.firstName = fields[1].GetString();
        employee.lastName = fields[2].GetString();
        employee.phone = fields[3].GetString();
        employee.location = fields[4].GetUInt32();
        employee.isActive = fields[5].GetBool();

        employeeMap[employee.id] = employee;
        ++count;
    }

    LOG_DEBUG("EmployeeManager::LoadEmployeeData: Loaded {} employees from database.", count);
}

std::unordered_map<std::uint32_t, EmployeeInformation> EmployeeManager::GetEmployeeMap() const
{
    return employeeMap;
}

bool EmployeeManager::AddNewEmployee(const EmployeeInformation& info)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);

    auto insert = connection->GetPreparedStatement(AMSPreparedStatement::DB_EI_INSERT_EMPLOYEE);    
    insert->SetString(0, info.firstName);
    insert->SetString(1, info.lastName);
    insert->SetString(2, info.phone);
    insert->SetUInt(3, info.location);
    insert->SetBool(4, info.isActive);

    bool success = connection->ExecutePreparedInsert(*insert);
    
    if (success)
    {
        const auto newId = GetLastInsertEmployeeID(info);
        employeeMap[newId] = info;
        emit GlobalSignals::instance()->SignalReloadEmployeeTable();
    }
    
    return success;
}

bool EmployeeManager::UpdateEmployee(const EmployeeInformation& info)
{
    // Implementation to update existing employee data in the database

    // UPDATE employees SET firstName = ?, lastName = ?, phone = ?, location = ?, isActive = ? WHERE id = ?

    ConnectionGuardAMS connection(ConnectionType::Sync);

    auto update = connection->GetPreparedStatement(AMSPreparedStatement::DB_EI_UPDATE_EMPLOYEE_BY_ID);
    update->SetString(0, info.firstName);
    update->SetString(1, info.lastName);
    update->SetString(2, info.phone);
    update->SetUInt(3, info.location);
    update->SetBool(4, info.isActive);
    update->SetUInt(5, info.id);

    bool success = connection->ExecutePreparedUpdate(*update);

    if (success)
    {
        employeeMap[info.id] = info;
        emit GlobalSignals::instance()->SignalReloadEmployeeTable();
    }

    return success;
}

bool EmployeeManager::DeleteEmployee(const std::uint32_t& employeeID)
{
    // DELETE FROM employees WHERE id = ?

    ConnectionGuardAMS connection(ConnectionType::Sync);

    auto deleteStmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_EI_DELETE_EMPLOYEE_BY_ID);
    deleteStmt->SetUInt(0, employeeID);
    bool success = connection->ExecutePreparedUpdate(*deleteStmt);

    if (success)
    {
        employeeMap.erase(employeeID);
        emit GlobalSignals::instance()->SignalReloadEmployeeTable();
    }

    return success;
}

std::vector<EmployeeInformation> EmployeeManager::LoadEmployeeDataForDetails(std::uint64_t ticketID)
{
    // SELECT employee_id FROM ticket_assignment WHERE ticket_id = ? AND is_current = 1

    std::vector<EmployeeInformation> employees;

    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_TA_SELECT_CURRENT_TICKET_ASSIGNMENT_BY_TICKET_ID);
    stmt->SetUInt64(0, ticketID);

    auto result = connection->ExecutePreparedSelect(*stmt);

    std::vector<std::uint32_t> employeeIDs;

    if (result.IsValid())
    {
        while (result.Next())
        {
            Field* fields = result.Fetch();
            if (!fields)
                break;

            std::uint32_t employeeID = fields[0].GetUInt32();
            employeeIDs.push_back(employeeID);
        }
    }

    // --- build vector with active flag ---
    employees.reserve(employeeMap.size());

    for (const auto& [id, info] : employeeMap)
    {
        EmployeeInformation copy = info;

        // reset
        copy.isActive = false;

        // check if this employee is assigned
        if (std::ranges::any_of(employeeIDs, [&](auto x) { return x == id; }))
            copy.isActive = true;

        employees.push_back(copy);
    }

    std::ranges::sort(employees, [](const auto& a, const auto& b)
      {
          if (a.isActive != b.isActive)
              return a.isActive > b.isActive;  // active first

          return a.lastName < b.lastName;
      });

    return employees;
}

void EmployeeManager::AddEmployeeAssignment(std::uint64_t ticketID, const std::uint32_t employeeID, const std::string& comment /*= ""*/)
{
    // INSERT INTO ticket_assignment (ticket_id, employee_id, assigned_at, comment_assigned, assigned_by_user_id) VALUES (?, ?, ?, ?)
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto insert = connection->GetPreparedStatement(AMSPreparedStatement::DB_TA_INSERT_NEW_ASSIGNMENT_WITH_COMMENT);
    insert->SetUInt64(0, ticketID);
    insert->SetUInt(1, employeeID);
    insert->SetCurrentDate(2);
    insert->SetString(3, comment);
    insert->SetUInt(4, GetUser().GetUserID());

    connection->ExecutePreparedInsert(*insert);
}

void EmployeeManager::RemoveEmployeeAssignment(std::uint64_t ticketID, const std::uint32_t employeeID, const std::string& comment /*= ""*/)
{
    // "UPDATE ticket_assignment SET unassigned_at = ?, is_current = 0, comment_unassigned = ?, unassigned_by_user_id = ? WHERE ticket_id = ? AND employee_id = ?

    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto update = connection->GetPreparedStatement(AMSPreparedStatement::DB_TA_DELETE_EMPLOYEE_ASSIGNMENT);
    update->SetCurrentDate(0);
    update->SetString(1, comment);
    update->SetUInt(2, GetUser().GetUserID());
    update->SetUInt64(3, ticketID);
    update->SetUInt(4, employeeID);

    connection->ExecutePreparedUpdate(*update);
}

std::uint32_t EmployeeManager::GetLastInsertEmployeeID(const EmployeeInformation& info)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto select = connection->GetPreparedStatement(AMSPreparedStatement::DB_EI_SELECT_EMPLOYEE_AFTER_INSERT);
    select->SetString(0, info.firstName);
    select->SetString(1, info.lastName);
    select->SetString(2, info.phone);
    select->SetUInt(3, info.location);
    auto result = connection->ExecutePreparedSelect(*select);

    if (!result.IsValid())
    return 0;

    auto fields = result.Fetch();
    return fields[0].GetUInt32();
}
