#include "ShowTicketManager.h"

#include <ranges>
#include <unordered_set>

#include "ConnectionGuard.h"
#include "CostUnitDataHandler.h"
#include "DatabaseTypes.h"
#include "pch.h"

ShowTicketManager::ShowTicketManager() {}


void ShowTicketManager::LoadTableTicketData()
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_TICKET_OVERVIEW_SELECT);

    auto result = connection->ExecutePreparedSelect(*stmt);

    if (!result.IsValid())
    {
        // TODO: log error
        return;
    }

    std::unordered_map<std::uint64_t, TicketRowData> rowMap;
    rowMap.reserve(128);

    while (result.Next())
    {
        Field* f = result.Fetch();

        const std::uint64_t ticketId = f[0].GetUInt64();

        auto it = rowMap.find(ticketId);
        if (it == rowMap.end())
        {
            TicketRowData row{};

            row.id = ticketId;
            row.title = f[1].GetString();
            if (!f[2].IsNull())
                row.area = f[2].GetString();
            else
                row.area = {};

            row.createdAt = f[3].GetDateTime();
            row.updatedAt = f[4].GetDateTime();

            if (!f[5].IsNull())
                row.status = static_cast<TicketStatus>(f[5].GetUInt8());
            else
                row.status = TicketStatus::TICKET_STATUS_NONE;

            row.priority = static_cast<TicketPriority>(f[6].GetUInt8());

            if (!f[7].IsNull())
            {
                const std::uint16_t costUnitId = f[7].GetUInt16();
                row.costUnitName = CostUnitDataHandler::instance().GetCostUnitNameByInternalId(costUnitId);
                std::string test{};
            }
            else
                row.costUnitName = {};

            {
                std::string reporterName = f[8].GetString();
                if (!f[9].IsNull())
                {
                    std::string reporterPhone = f[9].GetString();
                    if (!reporterPhone.empty())
                    {
                        reporterName.append(" - (");
                        reporterName.append(reporterPhone);
                        reporterName.push_back(')');
                    }
                }

                row.reporterName = std::move(reporterName);
            }

            if (!f[10].IsNull())
                row.machineName = f[10].GetString();
            else
                row.machineName.clear();

            if (!f[14].IsNull())
                row.lastComment = f[14].GetString();
            else
                row.lastComment.clear();

            // Insert into map and get iterator
            auto [newIt, inserted] = rowMap.emplace(ticketId, std::move(row));
            it = newIt;
        }

        // Employees (can be multiple rows per ticket)
        if (!f[11].IsNull() || !f[12].IsNull())
        {
            std::string firstName;
            std::string lastName;
            std::string phone;

            if (!f[11].IsNull())
                firstName = f[11].GetString();
            if (!f[12].IsNull())
                lastName = f[12].GetString();

            phone = f[13].GetString();

            if (!firstName.empty() || !lastName.empty())
            {
                std::string fullName;
                fullName.reserve(firstName.size() + 1 + lastName.size());

                if (!firstName.empty())
                {
                    fullName.append(firstName);
                    if (!lastName.empty())
                        fullName.push_back(' ');
                }

                if (!lastName.empty())
                    fullName.append(lastName);

                if (!phone.empty())
                {
                    fullName.append(" - (");
                    fullName.append(phone);
                    fullName.push_back(')');
                }
         
                auto& vec = it->second.employeeAssigned;
                if (std::ranges::find(vec, fullName) == vec.end())
                    vec.push_back(std::move(fullName));
            }
        }
    }

    // Move map content into vector for the model
    _ticketRowDataList.clear();
    _ticketRowDataList.reserve(rowMap.size());
    for (auto& row : rowMap | std::views::values)
        _ticketRowDataList.push_back(std::move(row));
}

ShowTicketData ShowTicketManager::LoadTicketDetails(std::uint64_t ticketID)
{
    ShowTicketData data;

    ConnectionGuardAMS connection(ConnectionType::Sync);

    LoadTicketData(ticketID, data.ticketInfo, connection);
    LoadTicketAssignments(ticketID, data.ticketAssignment, connection);
    LoadTicketAttachments(ticketID, data.ticketAttachment, connection);
    LoadTicketComments(ticketID, data.ticketComment, connection);
    LoadTicketStatusHistory(ticketID, data.ticketStatusHistory, connection);
    LoadCallerInformation(static_cast<std::uint16_t>(data.ticketInfo.reporterID), data.callerInfo, connection);
    LoadTicketSpareData(ticketID, data.sparePartsUsed, connection);

    std::unordered_set<uint32_t> employeeIds;

    for (const auto& a : data.ticketAssignment)
    {
        if (a.employeeID != 0)
            employeeIds.insert(a.employeeID);
    }
    
    for (auto id : employeeIds)
        LoadEmployees(id, data.employeeInfo, connection);

    LoadMachineData(data.ticketInfo.entityID, data.machineInfo, connection);

    _fullTicketData = data;

    return data;
}

const std::unordered_map<std::uint64_t, ShowTicketData>& ShowTicketManager::GetTicketDataMap() const
{    
    return ticketDataMap;
}

const std::vector<TicketRowData> ShowTicketManager::GetTableTicketVector() const
{
    return _ticketRowDataList;
}

ShowTicketData ShowTicketManager::GetTicketDataByID(std::uint64_t ticketID)
{
    const auto it = ticketDataMap.find(ticketID);
    if (it != ticketDataMap.end())
    {
        return it->second;
    }

    return ShowTicketData{}; 
}

void ShowTicketManager::LoadTicketData(std::uint64_t ticketID, TicketInformation& ticketInfo, const ConnectionGuardAMS& connection)
{
    // SELECT creator_user_id, created_at, current_status, cost_unit_id, area, reporter_id, entity.id, "
    // "title, description, priority, updated_at, closed_at, last_status_changed_at FROM tickets WHERE ID = ?

    auto stmt = connection->GetPreparedStatement(database::Implementation::AMSPreparedStatement::DB_TICKET_SELECT_ALL_TICKETS);
    stmt->SetUInt64(0, ticketID);

    auto result = connection->ExecutePreparedSelect(*stmt);

    if (!result.IsValid())
    {
        // log error
        return;
    }

    if (result.Next())
    {
        Field* fields = result.Fetch();
        ticketInfo.id = ticketID;
        ticketInfo.creatorUserID = fields[0].GetUInt32();
        ticketInfo.createdAt = fields[1].GetDateTime();
        ticketInfo.currentStatus = fields[2].GetUInt8();
        ticketInfo.costUnitID = fields[3].GetUInt16();
        ticketInfo.area = fields[4].GetString();
        ticketInfo.reporterID = fields[5].GetUInt64();
        ticketInfo.entityID = fields[6].GetUInt16();
        ticketInfo.title = fields[7].GetString();
        ticketInfo.description = fields[8].GetString();
        ticketInfo.priority = fields[9].GetUInt8();
        ticketInfo.updatedAt = fields[10].GetDateTime();
        ticketInfo.closedAt = fields[11].GetDateTime();
        ticketInfo.lastStatusChangeAt = fields[12].GetDateTime();
    }
}


void ShowTicketManager::LoadTicketAssignments(std::uint64_t ticketID, std::vector<TicketAssignmentInformation>& ticketAssignment, const ConnectionGuardAMS& connection)
{
    // SELECT ticket_id, employee_id, assigned_at, unassigned_at, is_current, comment_assigned, comment_unassigned, assigned_by_user_id, unassigned_by_user_id FROM ticket_assignment WHERE ticket_id = ?
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_TA_SELECT_TICKET_ASSIGNMENTS_BY_TICKET_ID);
    stmt->SetUInt64(0, ticketID);

    auto result = connection->ExecutePreparedSelect(*stmt);

    if (!result.IsValid())
    {
        // log error
        return;
    }

    while (result.Next())
    {
        Field* fields = result.Fetch();
        TicketAssignmentInformation assignmentInfo;
        assignmentInfo.ticketID = fields[0].GetUInt64();
        assignmentInfo.employeeID = fields[1].GetUInt32();
        assignmentInfo.assignedAt = fields[2].GetDateTime();
        assignmentInfo.unassignedAt = fields[3].GetDateTime();
        assignmentInfo.isCurrent = fields[4].GetBool();
        assignmentInfo.commentAssigned = fields[5].GetString();
        assignmentInfo.commentUnassigned = fields[6].GetString();
        assignmentInfo.assignedByUserID = fields[7].GetUInt32();
        assignmentInfo.unassignedByUserID = fields[8].GetUInt32();

        ticketAssignment.push_back(std::move(assignmentInfo));
    }
}

void ShowTicketManager::LoadTicketAttachments(std::uint64_t ticketID, std::vector<TicketAttachmentInformation>& ticketAttachment, const ConnectionGuardAMS& connection)
{

    // SELECT id, ticket_id, uploader_user_id, uploaded_at, original_filename, stored_file_name, file_path, mime_type,
    // file_size, description, is_deleted FROM ticket_attachment WHERE ticket_id = ?

    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_TATT_SELECT_TICKET_ATTACHMENTS_BY_TICKET_ID);
    stmt->SetUInt64(0, ticketID);

    auto result = connection->ExecutePreparedSelect(*stmt);

    if (!result.IsValid())
    {
        // log error
        return;
    }

    while (result.Next())
    {
        Field* fields = result.Fetch();
        TicketAttachmentInformation attachmentInfo;
        attachmentInfo.id = fields[0].GetUInt64();
        attachmentInfo.ticketID = fields[1].GetUInt64();
        attachmentInfo.uploaderUserID = fields[2].GetUInt32();
        attachmentInfo.uploadedAt = fields[3].GetDateTime();
        attachmentInfo.originalFilename = fields[4].GetString();
        attachmentInfo.storedFileName = fields[5].GetString();
        attachmentInfo.filePath = fields[6].GetString();

        if (!fields[7].IsNull())
            attachmentInfo.mimeType = fields[7].GetString();            
        else
            attachmentInfo.mimeType = {};

        if (!fields[8].IsNull())
            attachmentInfo.fileSize = fields[8].GetUInt64();
        else
            attachmentInfo.fileSize = 0;

        if (!fields[9].IsNull())
            attachmentInfo.description = fields[9].GetString();
        else
            attachmentInfo.description = {};
        attachmentInfo.isDeleted = fields[10].GetBool();

        ticketAttachment.push_back(std::move(attachmentInfo));
    }
}

void ShowTicketManager::LoadTicketComments(std::uint64_t ticketID, std::vector<TicketCommentInformation>& ticketComment, const ConnectionGuardAMS& connection)
{
    // SELECT id, ticket_id, author_user_id, created_at, updated_at, is_internal, is_deleted, message, delete_user_id, delete_at FROM ticket_comments WHERE ticket_id = ?

    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_TC_SELECT_TICKET_COMMENTS_BY_TICKET_ID);
    stmt->SetUInt64(0, ticketID);

    auto result = connection->ExecutePreparedSelect(*stmt);

    if (!result.IsValid())
    {

        return;
    }

    while (result.Next())
    {
        Field* fields = result.Fetch();
        TicketCommentInformation commentInfo;
        commentInfo.id = fields[0].GetUInt64();
        commentInfo.ticketID = fields[1].GetUInt64();
        commentInfo.authorUserID = fields[2].GetUInt32();
        commentInfo.createdAt = fields[3].GetDateTime();

        if (!fields[4].IsNull())
            commentInfo.updatedAt = fields[4].GetDateTime();
        else
            commentInfo.updatedAt = {};

        commentInfo.isInternal = fields[5].GetBool();
        commentInfo.isDeleted = fields[6].GetBool();
        commentInfo.message = fields[7].GetString();
        commentInfo.deleterUserID = fields[8].GetUInt32();        
        commentInfo.deleteAt = fields[9].GetDateTime();

        ticketComment.push_back(std::move(commentInfo));
    }
}

void ShowTicketManager::LoadTicketStatusHistory(std::uint64_t ticketID, std::vector<TicketStatusHistoryInformation>& ticketStatusHistory, const ConnectionGuardAMS& connection)
{

    // SELECT id, ticket_id, old_status, new_status, changed_at, changed_by_user, comment FROM ticket_status_history WHERE ticket_id = ?

    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_TSH_SELECT_TICKET_STATUS_HISTORY_BY_TICKET_ID);
    stmt->SetUInt64(0, ticketID);

    auto result = connection->ExecutePreparedSelect(*stmt);

    if (!result.IsValid())
    {
        // todo log
        return;
    }

    while (result.Next())
    {
        Field* fields = result.Fetch();
        TicketStatusHistoryInformation statusHistoryInfo;
        statusHistoryInfo.id = fields[0].GetUInt64();
        statusHistoryInfo.ticketID = fields[1].GetUInt64();

        if (!fields[2].IsNull())
            statusHistoryInfo.oldStatus = fields[2].GetUInt8();
        else
            statusHistoryInfo.oldStatus = 0;

        statusHistoryInfo.newStatus = fields[3].GetUInt8();
        statusHistoryInfo.changedAt = fields[4].GetDateTime();
        statusHistoryInfo.changedByUserID = fields[5].GetUInt32();

        if (!fields[6].IsNull())
            statusHistoryInfo.comment = fields[6].GetString();
        else
            statusHistoryInfo.comment = {};

        ticketStatusHistory.push_back(std::move(statusHistoryInfo));
    }
    
}

void ShowTicketManager::LoadCallerInformation(std::uint16_t callerID, CallerInformation& callerInfo, const ConnectionGuardAMS& connection)
{
    // SELECT id, department, phone, name, costUnit, location, is_active FROM caller_information WHERE id = ?

    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_CI_SELECT_CALLER_BY_ID);
    stmt->SetUInt(0, callerID);

    auto result = connection->ExecutePreparedSelect(*stmt);

    if (!result.IsValid())
    {

        return;
    }

    if (result.Next())
    {
        Field* fields = result.Fetch();
        callerInfo.id = fields[0].GetUInt64();

        if (!fields[1].IsNull())
            callerInfo.department = fields[1].GetString();
        else
            callerInfo.department = {};
        callerInfo.phone = fields[2].GetString();
        callerInfo.name = fields[3].GetString();

        if (!fields[4].IsNull())
            callerInfo.costUnitID = fields[4].GetUInt16();
        else
            callerInfo.costUnitID = 0;
        callerInfo.companyLocationID = fields[5].GetUInt32();
        callerInfo.is_active = fields[6].GetBool();
    }
}

void ShowTicketManager::LoadEmployees(std::uint32_t employeeID, std::vector<EmployeeInformation>& employeeInfo, const ConnectionGuardAMS& connection)
{
    // SELECT id, firstName, lastName, phone, location, isActive FROM employees WHERE id = ?

    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_EI_SELECT_EMPLOYEE_BY_ID);
    stmt->SetUInt(0, employeeID);

    auto result = connection->ExecutePreparedSelect(*stmt);

    if (!result.IsValid())
    {
        return;
    }

    while (result.Next())
    {
        Field* fields = result.Fetch();
        EmployeeInformation info;
        info.id = fields[0].GetUInt32();
        info.firstName = fields[1].GetString();
        info.lastName = fields[2].GetString();
        info.phone = fields[3].GetString();
        info.location = fields[4].GetUInt32();
        info.isActive = fields[5].GetBool();
        
        // Avoid duplicates
        auto it = std::ranges::find_if(employeeInfo, [&](const EmployeeInformation& e) { return e.id == info.id; });

        if (it == employeeInfo.end())
            employeeInfo.push_back(std::move(info));
    }
}

void ShowTicketManager::LoadMachineData(std::uint32_t machineID, MachineInformation& machineInfo, const ConnectionGuardAMS& connection)
{
    // SELECT ID, CostUnitID, MachineTypeID, LineID, ManufacturerID, MachineName, MachineNumber, ManufacturerMachineNumber, RoomNumber, MoreInformation, location FROM machine_list WHERE ID = ?

    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_ML_SELECT_MACHINE_BY_ID);
    stmt->SetUInt(0, machineID);

    auto result = connection->ExecutePreparedSelect(*stmt);

    if (!result.IsValid())
    {
        return;
    }

    if (result.Next())
    {
        
        Field* fields = result.Fetch();
        machineInfo.ID = fields[0].GetInt32();
        if (!fields[1].IsNull())
            machineInfo.CostUnitID = fields[1].GetUInt16();
        else
            machineInfo.CostUnitID = 0;

        if (!fields[2].IsNull())
            machineInfo.MachineTypeID = fields[2].GetUInt32();
        else
            machineInfo.MachineTypeID = 0;

        if (!fields[3].IsNull())
            machineInfo.LineID = fields[3].GetUInt32();
        else
            machineInfo.LineID = 0;

        if (!fields[4].IsNull())
            machineInfo.ManufacturerID = fields[4].GetUInt32();
        else
            machineInfo.ManufacturerID = 0;

        if (!fields[5].IsNull())
            machineInfo.MachineName = fields[5].GetString();
        else
            machineInfo.MachineName = {};

        if (!fields[6].IsNull())
            machineInfo.MachineNumber = fields[6].GetString();
        else
            machineInfo.MachineNumber = {};

        if (!fields[7].IsNull())
            machineInfo.ManufacturerMachineNumber = fields[7].GetString();
        else
            machineInfo.ManufacturerMachineNumber = {};

        if (!fields[8].IsNull())
            machineInfo.RoomID = fields[8].GetUInt16();
        else
            machineInfo.RoomID = 0;

        if (!fields[9].IsNull())
            machineInfo.MoreInformation = fields[9].GetString();
        else
            machineInfo.MoreInformation = {};

        if (!fields[10].IsNull())
            machineInfo.locationID = fields[10].GetUInt32();
        else
            machineInfo.locationID = 0;
    }
}

void ShowTicketManager::LoadTicketSpareData(std::uint64_t ticketID, std::vector<SparePartsTable>& sparePartsUsed,
                                            const ConnectionGuardAMS& connection)
{
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_TSPU_SELECT_SPARE_PARTS_USED_BY_TICKET_ID);
    stmt->SetUInt64(0, ticketID);

    auto result = connection->ExecutePreparedSelect(*stmt);

    if (!result.IsValid())
        return;

    std::vector<TicketSparePartUsedInformation> rows;

    rows.reserve(16);

    while (result.Next())
    {
        Field* fields = result.Fetch();

        TicketSparePartUsedInformation row;

        row.id = fields[0].GetUInt64();
        row.ticketID = fields[1].GetUInt64();
        row.machineID = fields[2].GetUInt32();
        row.articleID = fields[3].GetUInt64();
        row.quantity = fields[4].GetUInt16();
        row.unit = fields[5].GetString();
        row.note = fields[6].GetString();
        row.createdByUserID = fields[7].GetUInt64();
        row.createdByUserName = fields[8].GetString();
        row.createdAt = fields[9].GetDateTime();

        rows.push_back(row);
    }


    if (rows.empty())
        return;

    sparePartsUsed.reserve(rows.size());

    ConnectionGuardIMS connectAD(ConnectionType::Sync);

    for (const auto& sparePart : rows)
    {
        SparePartsTable tableEntry;
        tableEntry.spareData = sparePart;

        auto stmtan = connectAD->GetPreparedStatement(IMSPreparedStatement::DB_AD_SELECT_ARTICLE_NAME_BY_ID);
        stmtan->SetUInt64(0, sparePart.articleID);

        auto resultan = connectAD->ExecutePreparedSelect(*stmtan);
        if (resultan.IsValid() && resultan.Next())
        {
            Field* fields = resultan.Fetch();
            tableEntry.articleName = fields[0].GetString();
        }
        else
        {
            tableEntry.articleName = "Unknown Article";
        }

        sparePartsUsed.push_back(tableEntry);
    }
}

TicketDelta ShowTicketManager::LoadTableTicketDelta(const std::string& sinceDb)
{
    TicketDelta delta;

    // Capture first to avoid missing updates happening during the select.
    delta.newSyncPoint = Util::GetCurrentSystemPointTime();

    ConnectionGuardAMS connection(ConnectionType::Sync);

    // 1) Upserts (new/changed tickets)
    {
        auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_TICKET_OVERVIEW_SELECT_SINCE);
        stmt->SetString(0, sinceDb);

        auto result = connection->ExecutePreparedSelect(*stmt);
        if (!result.IsValid())
        {
            return delta;
        }

        std::unordered_map<std::uint64_t, TicketRowData> rowMap;
        rowMap.reserve(128);

        while (result.Next())
        {
            Field* f = result.Fetch();

            const std::uint64_t ticketId = f[0].GetUInt64();

            auto it = rowMap.find(ticketId);
            if (it == rowMap.end())
            {
                TicketRowData row{};

                row.id = ticketId;
                row.title = f[1].GetString();
                row.area = !f[2].IsNull() ? f[2].GetString() : std::string{};

                row.createdAt = f[3].GetDateTime();
                row.updatedAt = f[4].GetDateTime();

                row.status =
                    !f[5].IsNull() ? static_cast<TicketStatus>(f[5].GetUInt8()) : TicketStatus::TICKET_STATUS_NONE;

                row.priority = static_cast<TicketPriority>(f[6].GetUInt8());

                if (!f[7].IsNull())
                {
                    const std::uint16_t costUnitId = f[7].GetUInt16();
                    row.costUnitName = CostUnitDataHandler::instance().GetCostUnitNameByInternalId(costUnitId);
                }
                else
                {
                    row.costUnitName = {};
                }

                {
                    std::string reporterName = !f[8].IsNull() ? f[8].GetString() : std::string{};
                    if (!f[9].IsNull())
                    {
                        std::string reporterPhone = f[9].GetString();
                        if (!reporterPhone.empty())
                        {
                            reporterName.append(" - (");
                            reporterName.append(reporterPhone);
                            reporterName.push_back(')');
                        }
                    }
                    row.reporterName = std::move(reporterName);
                }

                row.machineName = !f[10].IsNull() ? f[10].GetString() : std::string{};

                auto [newIt, inserted] = rowMap.emplace(ticketId, std::move(row));
                it = newIt;
            }

            // Current employees (can be multiple rows per ticket)
            if (!f[11].IsNull() || !f[12].IsNull())
            {
                std::string firstName = !f[11].IsNull() ? f[11].GetString() : std::string{};
                std::string lastName = !f[12].IsNull() ? f[12].GetString() : std::string{};
                std::string phone = !f[13].IsNull() ? f[13].GetString() : std::string{};

                if (!firstName.empty() || !lastName.empty())
                {
                    std::string fullName;
                    fullName.reserve(firstName.size() + 1 + lastName.size() + 16);

                    if (!firstName.empty())
                    {
                        fullName.append(firstName);
                        if (!lastName.empty())
                            fullName.push_back(' ');
                    }

                    if (!lastName.empty())
                        fullName.append(lastName);

                    if (!phone.empty())
                    {
                        fullName.append(" - (");
                        fullName.append(phone);
                        fullName.push_back(')');
                    }

                    auto& vec = it->second.employeeAssigned;
                    if (std::ranges::find(vec, fullName) == vec.end())
                        vec.push_back(std::move(fullName));
                }
            }
        }

        delta.upserts.reserve(rowMap.size());
        for (auto& row : rowMap | std::views::values)
            delta.upserts.push_back(std::move(row));
    }

    // 2) Removed (closed/resolved/deleted tickets)
    {
        auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_TICKET_OVERVIEW_SELECT_REMOVED_SINCE);
        stmt->SetString(0, sinceDb);
        stmt->SetUInt(1, static_cast<std::uint32_t>(TicketStatus::TICKET_STATUS_RESOLVED));
        stmt->SetUInt(2, static_cast<std::uint32_t>(TicketStatus::TICKET_STATUS_CLOSED));

        auto result = connection->ExecutePreparedSelect(*stmt);
        if (result.IsValid())
        {
            while (result.Next())
            {
                Field* f = result.Fetch();
                delta.removedIds.push_back(f[0].GetUInt64());
            }
        }
    }

    // De-dup removed IDs
    if (delta.removedIds.size() > 1)
    {
        std::ranges::sort(delta.removedIds);
        delta.removedIds.erase(std::ranges::unique(delta.removedIds).begin(), delta.removedIds.end());
    }

    return delta;
}
