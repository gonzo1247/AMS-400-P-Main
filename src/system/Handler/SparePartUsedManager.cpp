#include "pch.h"
#include "SparePartUsedManager.h"

#include "DatabaseTypes.h"
#include "ConnectionGuard.h"
#include "DatabaseDefines.h"

SparePartUsedManager::SparePartUsedManager()
{}

void SparePartUsedManager::LoadAllSparePartsData() {}

std::vector<SparePartsTable> SparePartUsedManager::LoadSparePartsDataForTicket(std::uint64_t ticketID)
{
    //         0     1           2           3          4        5     6        7           8               9 
    // SELECT id, ticket_id, machine_id, article_id, quantity, unit, note, created_by, created_by_name, created_at "
    // FROM ticket_spare_parts_used WHERE ticket_id = ? AND is_deleted = 0",

    ConnectionGuardAMS connection(database::ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_TSPU_SELECT_SPARE_PARTS_USED_BY_TICKET_ID);
    stmt->SetUInt64(0, ticketID);

    auto result = connection->ExecutePreparedSelect(*stmt);

    if (!result.IsValid())
        return {};

    std::vector<TicketSparePartUsedInformation> rows;
    std::vector<SparePartsTable> rowResult;
    rows.reserve(16);
    rowResult.reserve(16);

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

    return LoadArticleName(rows);
}

bool SparePartUsedManager::SoftDeleteSparePartUsedById(std::uint64_t rowId, std::uint64_t deletedBy)
{
    ConnectionGuardAMS connection(database::ConnectionType::Sync);
    if (!connection)
    {
        LOG_DEBUG("SoftDeleteSparePartUsedById: No DB connection");
        return false;
    }

    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_TSPU_SOFT_DELETE_BY_ID);
    stmt->SetUInt64(0, deletedBy);
    stmt->SetUInt64(1, rowId);

    const bool ok = connection->ExecutePreparedUpdate(*stmt);
    if (!ok)
    {
        LOG_DEBUG("SoftDeleteSparePartUsedById: Update failed (rowId={}, deletedBy={})", rowId, deletedBy);
    }

    return ok;
}

std::optional<std::uint64_t> SparePartUsedManager::InsertSparePartUsed(std::uint64_t ticketId, std::uint32_t machineId,
std::uint64_t articleId, std::uint16_t quantity, const std::string& unit, const std::string& note, std::uint64_t createdBy, const std::string& createdByName)
{
    ConnectionGuardAMS connection(database::ConnectionType::Sync);
    if (!connection)
    {
        LOG_DEBUG("InsertSparePartUsed: No DB connection");
        return std::nullopt;
    }

    // ticket_id, machine_id, article_id, quantity, unit, note, "
    // created_by, created_by_name
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_TSPU_INSERT_NEW_SPARE_PART_USED);
    stmt->SetUInt64(0, ticketId);
    stmt->SetUInt(1, machineId);
    stmt->SetUInt64(2, articleId);
    stmt->SetUInt(3, quantity);
    stmt->SetString(4, unit);
    stmt->SetString(5, note);
    stmt->SetUInt64(6, createdBy);
    stmt->SetString(7, createdByName);

    if (!connection->ExecutePreparedInsert(*stmt))
    {
        LOG_DEBUG("InsertSparePartUsed: Insert failed (ticket={}, machine={}, article={})", ticketId, machineId,
                  articleId);
        return std::nullopt;
    }

    return connection->GetLastInsertId();
}

std::vector<SparePartsTable> SparePartUsedManager::LoadArticleName(const std::vector<TicketSparePartUsedInformation>& sparePartsUsedList)
{
    if (sparePartsUsedList.empty())
        return {};

    std::vector<SparePartsTable> resultData;
    resultData.reserve(sparePartsUsedList.size());

    ConnectionGuardIMS connection(ConnectionType::Sync);

    for (const auto& sparePart : sparePartsUsedList)
    {
        SparePartsTable tableEntry;
        tableEntry.spareData = sparePart;

        auto stmt = connection->GetPreparedStatement(IMSPreparedStatement::DB_AD_SELECT_ARTICLE_NAME_BY_ID);
        stmt->SetUInt64(0, sparePart.articleID);

        auto result = connection->ExecutePreparedSelect(*stmt);
        if (result.IsValid() && result.Next())
        {
            Field* fields = result.Fetch();
            tableEntry.articleName = fields[0].GetString();
        }
        else
        {
            tableEntry.articleName = "Unknown Article";
        }

        resultData.push_back(tableEntry);

    }

    return resultData;
}


