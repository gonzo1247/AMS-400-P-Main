
#include "CreateTicketManager.h"

#include "ConnectionGuard.h"
#include "DatabaseTypes.h"
#include "UserManagement.h"
#include "pch.h"

CreateTicketManager::CreateTicketManager() : _lastTicketID(0)
{}

bool CreateTicketManager::SaveTicket(const TicketInformation& ticketInfo, const TicketAssignmentInformation& assignmentInfo)
{ 
    return CreateNewTicket(ticketInfo) && CreateNewTicketAssignment(assignmentInfo);
}

bool CreateTicketManager::CreateNewTicket(const TicketInformation& ticketInfo)
{
    // INSERT INTO tickets (creator_user_id, current_status, cost_unit_id, area, reporter_name, reporter_phone, reporter_id, entity_id, title, description, priority)
    ConnectionGuardAMS connection(database::ConnectionType::Sync);
    auto insert = connection->GetPreparedStatement(AMSPreparedStatement::DB_TICKET_INSERT_NEW_TICKET);
    insert->SetUInt(0, GetUser().GetUserID());
    insert->SetUInt(1, ticketInfo.currentStatus);
    insert->SetUInt(2, ticketInfo.costUnitID);
    insert->SetString(3, ticketInfo.area);
    insert->SetString(4, ticketInfo.reporterName);
    insert->SetString(5, ticketInfo.reporterPhone);
    insert->SetUInt64(6, ticketInfo.reporterID);
    insert->SetUInt(7, ticketInfo.entityID);
    insert->SetString(8, ticketInfo.title);
    insert->SetString(9, ticketInfo.description);
    insert->SetUInt(10, ticketInfo.priority);

    if (!connection->ExecutePreparedInsert(*insert))
    {
        LOG_ERROR("CreateTicketManager::CreateNewTicket: INSERT failed");
        return false;
    }

    _lastTicketID = static_cast<std::uint32_t>(connection->GetLastInsertId());

    LOG_DEBUG("CreateTicketManager::CreateNewTicket: Created new ticket with ID {}", _lastTicketID);

    return true;
}

bool CreateTicketManager::CreateNewTicketAssignment(const TicketAssignmentInformation& assignmentInfo)
{
    // INSERT INTO ticket_assignment (ticket_id, employee_id, assigned_at) VALUES (?, ?, ?)

    if (_lastTicketID == 0)
    {
        LOG_ERROR("CreateTicketManager::CreateNewTicketAssignment: No valid ticket ID to assign.");
        return false;
    }

    if (assignmentInfo.employeeID == 0)
    {
        LOG_ERROR("CreateTicketManager::CreateNewTicketAssignment: No valid employee ID to assign ticket ID {}.", _lastTicketID);
        return true;
    }

    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto insert = connection->GetPreparedStatement(AMSPreparedStatement::DB_TA_INSERT_NEW_TICKET_ASSIGNMENT);
    
    insert->SetUInt(0, _lastTicketID);
    insert->SetUInt(1, assignmentInfo.employeeID);
    insert->SetCurrentDate(2);
    
    connection->ExecutePreparedInsert(*insert);
    LOG_DEBUG("CreateTicketManager::CreateNewTicketAssignment: Assigned ticket ID {} to employee ID {}", _lastTicketID, assignmentInfo.employeeID);

    return true;
}
