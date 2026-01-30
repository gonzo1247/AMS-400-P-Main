#include "pch.h"
#include "TicketReportManager.h"

#include "ConnectionGuard.h"
#include "DatabaseTypes.h"

TicketReportManager::TicketReportManager()
{}

TicketReportData TicketReportManager::LoadTicketReport(std::uint64_t ticketID)
{
    // Load the ticket report from the database

    //         0       1           2          3               4           5             6           7
    // SELECT id, ticket_id, report_html, report_plain, created_by, created_by_name, created_at, updated_at FROM ticket_report WHERE ticket_id = ?

    TicketReportData reportData;

    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_TR_SELECT_TICKET_REPORTS_BY_TICKET_ID);
    stmt->SetUInt64(0, ticketID);

    auto result = connection->ExecutePreparedSelect(*stmt);

    if (!result.IsValid())
        return reportData;

    if (result.Next())
    {
        Field* fields = result.Fetch();
        reportData.id = fields[0].GetUInt64();
        reportData.ticketID = fields[1].GetUInt64();
        reportData.reportHTML = fields[2].GetString();
        reportData.reportPlain = fields[3].GetString();
        reportData.createdByUserID = fields[4].GetUInt32();
        reportData.createdByUserName = fields[5].GetString();
        reportData.createdAt = fields[6].GetDateTime();
        reportData.updatedAt = fields[7].GetOptionalDateTime();
    }

    return reportData;
}

bool TicketReportManager::SaveNewReport(const TicketReportData& data)
{
    // INSERT INTO ticket_report (ticket_id, report_html, report_plain, created_by, created_by_name) VALUES (?, ?, ?, ?, ?)

    ConnectionGuardAMS connection(ConnectionType::Async);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_TR_INSERT_NEW_TICKET_REPORT);
    stmt->SetUInt64(0, data.ticketID);
    stmt->SetString(1, data.reportHTML);
    stmt->SetString(2, data.reportPlain);
    stmt->SetUInt64(3, data.createdByUserID);
    stmt->SetString(4, data.createdByUserName);

    return connection->ExecutePreparedInsert(*stmt);
}

bool TicketReportManager::SaveReportUpdate(const TicketReportData& data)
{
    // UPDATE ticket_report SET report_html = ?, report_plain = ? WHERE id = ?

    ConnectionGuardAMS connection(ConnectionType::Async);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_TR_UPDATE_TICKET_REPORT_BY_ID);
    stmt->SetString(0, data.reportHTML);
    stmt->SetString(1, data.reportPlain);
    stmt->SetUInt64(2, data.id);

    return connection->ExecutePreparedUpdate(*stmt);
}
