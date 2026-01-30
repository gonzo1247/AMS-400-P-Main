#include "pch.h"
#include "ContractorVisitManager.h"

#include "ConnectionGuard.h"
#include "ContractorVisitModel.h"
#include "ContractorVisitWorkerModel.h"
#include "UserManagement.h"

ContractorVisitManager::ContractorVisitManager() {}

void ContractorVisitManager::LoadContractorVisitData()
{
    //         0        1               2                   3             4            5           6
    // SELECT id, company_name, company_external_id, contact_person, worker_name,  arrival_at, departure_at "
    //     7        8               9              10          11    12             13          14          15
    //"activity, location, reachable_phone, reachable_note, status, note, created_by_user_id, created_at, updated_at "
    //"FROM contractor_visit"

    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_CV_SELECT_CONTRACTOR_VISITS);

    auto result = connection->ExecutePreparedSelect(*stmt);

    if (!result.IsValid())
        return;

    int count = 0;

    _contractorVisit.clear();
    _contractorVisit.reserve(128);

    while (result.Next())
    {
        Field* fields = result.Fetch();

        ContractorVisitInformation row;

        row.id = fields[0].GetUInt64();
        row.contractorCompany = fields[1].GetString();
        row.contractorCompanyID = fields[2].GetUInt32();
        row.contactPerson = fields[3].GetString();
        row.workerName = fields[4].GetString();
        row.arrival_at = fields[5].GetDateTime();
        row.departure_at = fields[6].GetDateTime();
        row.activity = fields[7].GetString();
        row.location = fields[8].GetString();
        row.contactPhone = fields[9].GetString();
        row.contactNote = fields[10].GetString();
        row.status = fields[11].GetUInt8();
        row.note = fields[12].GetString();
        row.creatorUserID = fields[13].GetUInt32();
        row.createdAt = fields[14].GetDateTime();
        row.updatedAt = fields[15].GetDateTime();

        _contractorVisit.push_back(row);
        ++count;

        LOG_DEBUG("Loaded data status is {}", static_cast<int>(row.status));
    }

    LOG_DEBUG("Loaded {} contractor visit records", count);

}

std::vector<ContractorVisitInformation> ContractorVisitManager::GetActiveContractorVisits()
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_CV_SELECT_OVERVIEW);
    // SELECT v.id, v.company_name, v.company_external_id, v.contact_person, v.worker_names_manual, v.arrival_at, "
    // v.departure_at, v.activity, v.location, v.reachable_phone, v.reachable_note, v.status, v.note, "
    // v.created_by_user_id, v.created_at, v.updated_at "
    // FROM contractor_visit v WHERE (v.arrival_at >= CURDATE()) OR (v.departure_at IS NULL) ORDER BY (v.departure_at IS NULL) DESC, v.arrival_at ASC

    auto result = connection->ExecutePreparedSelect(*stmt);

    if (!result.IsValid())
        return {};

    _activeVisit.clear();
    _visitByID.clear();

    while (result.Next())
    {
        ContractorVisitInformation visit{};
        Field* fields = result.Fetch();

        visit.id = fields[0].GetUInt64();
        visit.contractorCompany = fields[1].GetString();
        visit.contractorCompanyID = fields[2].GetUInt32();
        visit.contactPerson = fields[3].GetString();
        visit.workerName = fields[4].GetString(); // worker_names_manual
        visit.arrival_at = fields[5].GetDateTime();

        if (!fields[6].IsNull())
            visit.departure_at = fields[6].GetDateTime();

        visit.activity = fields[7].GetString();
        visit.location = fields[8].GetString();
        visit.contactPhone = fields[9].GetString();
        visit.contactNote = fields[10].GetString();
        visit.status = fields[11].GetUInt8();
        visit.note = fields[12].GetString();
        visit.creatorUserID = fields[13].GetUInt32();
        visit.createdAt = fields[14].GetDateTime();

        if (!fields[15].IsNull())
            visit.updatedAt = fields[15].GetDateTime();


        _activeVisit.push_back(visit);
        auto [it, inserted] = _visitByID.emplace(visit.id, visit);
        if (!inserted)
        {
            LOG_DEBUG("Duplicate visit id detected: {}", visit.id);
            it->second = visit;
        }

        LOG_DEBUG("Active visit loaded: ID {}, status {}", visit.id, static_cast<int>(visit.status));
    }

    return _activeVisit;
}

ContractorVisitInformation ContractorVisitManager::GetContractorVisitByID(std::uint64_t visitID)
{
    auto itr = _visitByID.find(visitID);
    if (itr != _visitByID.end())
        return itr->second;

    return LoadContractorVisitByID(visitID);
}

ContractorVisitInformation ContractorVisitManager::LoadContractorVisitByID(std::uint64_t visitID)
{
    //          0        1               2                   3             4            5           
    //  SELECT id, company_name, company_external_id, company_source, contact_person, arrival_at, "
    //      6           7        8               9              10          11      12          13                14        15
    //  departure_at, activity, location, reachable_phone, reachable_note, status, note, created_by_user_id, created_at, updated_at "
    //  FROM contractor_visit WHERE id = ?



    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_CV_SELECT_BY_ID);
    stmt->SetUInt64(0, visitID);

    auto result = connection->ExecutePreparedSelect(*stmt);
    if (!result.IsValid() || !result.Next())
        return {};

    ContractorVisitInformation visit{};
    Field* fields = result.Fetch();

    visit.id = fields[0].GetUInt64();
    visit.contractorCompany = fields[1].GetString();
    visit.contractorCompanyID = fields[2].GetUInt32();
    visit.contactPerson = fields[4].GetString();
    visit.arrival_at = fields[5].GetDateTime();

    if (!fields[6].IsNull())
        visit.departure_at = fields[6].GetDateTime();

    visit.activity = fields[7].GetString();
    visit.location = fields[8].GetString();
    visit.contactPhone = fields[9].GetString();
    visit.contactNote = fields[10].GetString();
    visit.status = fields[11].GetUInt8();
    visit.note = fields[12].GetString();
    visit.creatorUserID = fields[13].GetUInt32();
    visit.createdAt = fields[14].GetDateTime();

    if (!fields[15].IsNull())
        visit.updatedAt = fields[15].GetDateTime();

    _visitByID[visit.id] = visit;

    return visit;
}

std::vector<ContractorVisitWorkerInfo> ContractorVisitManager::GetWorkersForVisit(std::uint64_t visitID)
{
    std::vector<ContractorVisitWorkerInfo> workers;
    ConnectionGuardAMS connection(ConnectionType::Sync);

    //              0                        1                      2                   3           4
    // SELECT cvw.contractor_worker_id, cw.company_name, cw.company_external_id, cw.first_name, cw.last_name, "
    //      5       6
    // cw.phone, cw.email "
    // FROM contractor_visit_worker AS cvw "
    // LEFT JOIN contractor_worker AS cw ON cw.id = cvw.contractor_worker_id "
    //  WHERE contractor_visit_id = ?

    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_CVW_SELECT_WORKERS_BY_CONTRACTOR_VISIT_ID);
    stmt->SetUInt64(0, visitID);

    auto result = connection->ExecutePreparedSelect(*stmt);

    if (!result.IsValid())
        return workers;

    while (result.Next())
    {
        Field* fields = result.Fetch();

        ContractorVisitWorkerInfo workerInfo;

        workerInfo.workerId = fields[0].GetUInt64();
        workerInfo.company = fields[1].GetString();
        workerInfo.firstName = fields[3].GetString();
        workerInfo.lastName = fields[4].GetString();
        workerInfo.phone = fields[5].GetString();

        workers.push_back(workerInfo);
    }

    return workers;
}

std::uint64_t ContractorVisitManager::AddNewWorker(const ContractorVisitWorkerInfo& workerInfo, std::uint64_t companyID /*= 0*/)
{
    // INSERT INTO contractor_worker (company_name, company_external_id, first_name, last_name, phone, email, note, is_active) "
    // VALUES (?, ?, ?, ?, ?, ?, ?, ?)

    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_CW_INSERT_NEW_CONTRACTOR_WORKER);
    stmt->SetString(0, workerInfo.company);
    stmt->SetUInt64(1, companyID);
    stmt->SetString(2, workerInfo.firstName);
    stmt->SetString(4, workerInfo.lastName);
    stmt->SetString(5, workerInfo.phone);
    stmt->SetString(6, "");  // email
    stmt->SetString(7, "");  // note
    stmt->SetBool(8, true);  // is_active

    connection->ExecutePreparedInsert(*stmt);

    return connection->GetLastInsertId();
}

bool ContractorVisitManager::UpdateVisit(const ContractorVisitInformation& visitInfo,
                                         std::unordered_set<std::uint64_t> workerToAdd,
                                         std::unordered_set<std::uint64_t> workerToRemove)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    NormalizeWorkerChanges(workerToAdd, workerToRemove);

    try
    {
        connection->BeginTransaction();

        {
            auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_CV_UPDATE_CONTRACTOR_VISIT_BY_ID);

            stmt->SetString(0, visitInfo.contactPerson);
            stmt->SetSystemPointTime(1, visitInfo.arrival_at);

            if (visitInfo.departure_at == SystemTimePoint{})
                stmt->SetNull(2);
            else
                stmt->SetSystemPointTime(2, visitInfo.departure_at);

            stmt->SetString(3, visitInfo.activity);
            stmt->SetString(4, visitInfo.location);
            stmt->SetString(5, visitInfo.contactPhone);
            stmt->SetString(6, visitInfo.contactNote);
            stmt->SetUInt(7, static_cast<std::uint8_t>(visitInfo.status));
            stmt->SetString(8, visitInfo.note);
            stmt->SetUInt64(9, visitInfo.id);

            if (!connection->ExecutePreparedUpdate(*stmt))
            {
                connection->Rollback();
                return false;
            }
        }

        if (!AddWorkerToVisit(*connection, workerToAdd, visitInfo.id))
        {
            connection->Rollback();
            return false;
        }

        if (!RemoveWorkerFromVisit(*connection, workerToRemove, visitInfo.id))
        {
            connection->Rollback();
            return false;
        }

        connection->Commit();
        return true;
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR("UpdateVisit failed with exception: {}", ex.what());

        try
        {
            connection->Rollback();
            LOG_SQL("Transaction rolled back successfully");
        }
        catch (const std::exception& rbEx)
        {
            LOG_ERROR("Rollback failed: {}", rbEx.what());
        }
        catch (...)
        {
            LOG_ERROR("Rollback failed with unknown exception");
        }

        return false;
    }
    catch (...)
    {
        LOG_ERROR("UpdateVisit failed with unknown exception");

        try
        {
            connection->Rollback();
            LOG_SQL("Transaction rolled back successfully");
        }
        catch (const std::exception& rbEx)
        {
            LOG_ERROR("Rollback failed: {}", rbEx.what());
        }
        catch (...)
        {
            LOG_ERROR("Rollback failed with unknown exception");
        }

        return false;
    }
}


bool ContractorVisitManager::AddWorkerToVisit(database::DatabaseConnection& connection, const std::unordered_set<std::uint64_t>& workerToAdd, std::uint64_t visitID)
{
    // REPLACE INTO contractor_visit_worker (contractor_visit_id, contractor_worker_id) VALUES (?, ?)
    
    if (workerToAdd.empty())
        return true;

    auto stmt = connection.GetPreparedStatement(AMSPreparedStatement::DB_CVW_REPLACE_WORKERS_FOR_CONTRACTOR_VISIT_ID);

    for (const auto& workerID : workerToAdd)
    {
        stmt->SetUInt64(0, visitID);
        stmt->SetUInt64(1, workerID);

        if (!connection.ExecutePreparedInsert(*stmt))
            return false;
    }

    return true;
}

bool ContractorVisitManager::RemoveWorkerFromVisit(database::DatabaseConnection& connection, const std::unordered_set<std::uint64_t>& workerToRemove, std::uint64_t visitID)
{
    // DELETE FROM contractor_visit_worker WHERE contractor_visit_id = ? AND contractor_worker_id = ?
    if (workerToRemove.empty())
        return true;

    auto stmt = connection.GetPreparedStatement(AMSPreparedStatement::DB_CVW_DELETE_WORKERS_FOR_CONTRACTOR_VISIT_ID);

    for (const auto& workerID : workerToRemove)
    {
        stmt->SetUInt64(0, visitID);
        stmt->SetUInt64(1, workerID);

        (void)connection.ExecutePreparedDelete(*stmt);
    }

    return true;
}

bool ContractorVisitManager::AddNewVisit(const ContractorVisitInformation& visitInfo, std::unordered_set<std::uint64_t> workerToAdd, std::unordered_set<std::uint64_t> workerToRemove)
{
    // INSERT INTO contractor_visit (company_name, company_external_id, company_source, contact_person, arrival_at, departure_at, activity, location, reachable_phone, reachable_note, status, 
    // note, created_by_user_id VALUES (?, ?, ?, ?, ?, ?, ?, ?, ? ,? ,? , ?, ?)

    ConnectionGuardAMS connection(ConnectionType::Sync);
    NormalizeWorkerChanges(workerToAdd, workerToRemove);

    try
    {
        connection->BeginTransaction();

        auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_CV_INSERT_NEW_CONTRACTOR_VISIT);
        stmt->SetString(0, visitInfo.contractorCompany);
        stmt->SetUInt64(1, visitInfo.contractorCompanyID);
        stmt->SetUInt(2, 0);
        stmt->SetString(3, visitInfo.contactPerson);
        stmt->SetSystemPointTime(4, visitInfo.arrival_at);
        if (visitInfo.departure_at == SystemTimePoint{})
            stmt->SetNull(5);
        else
            stmt->SetSystemPointTime(5, visitInfo.departure_at);
        stmt->SetString(6, visitInfo.activity);
        stmt->SetString(7, visitInfo.location);
        stmt->SetString(8, visitInfo.contactPhone);
        stmt->SetString(9, visitInfo.contactNote);
        stmt->SetUInt(10, static_cast<std::uint32_t>(visitInfo.status));
        stmt->SetString(11, visitInfo.note);
        stmt->SetUInt64(12, GetUser().GetUserID());

        if (!connection->ExecutePreparedInsert(*stmt))
        {
            connection->Rollback();
            return false;
        }

        std::uint64_t visitID = connection->GetLastInsertId();

        if (!AddWorkerToVisit(*connection, workerToAdd, visitID))
        {
            connection->Rollback();
            return false;
        }

        if (!RemoveWorkerFromVisit(*connection, workerToRemove, visitID))
        {
            connection->Rollback();
            return false;
        }

        connection->Commit();
        return true;

    }
    catch (const std::exception& ex)
    {
        LOG_ERROR("UpdateVisit failed with exception: {}", ex.what());

        try
        {
            connection->Rollback();
            LOG_SQL("Transaction rolled back successfully");
        }
        catch (const std::exception& rbEx)
        {
            LOG_ERROR("Rollback failed: {}", rbEx.what());
        }
        catch (...)
        {
            LOG_ERROR("Rollback failed with unknown exception");
        }

        return false;
    }
    catch (...)
    {
        LOG_ERROR("UpdateVisit failed with unknown exception");

        try
        {
            connection->Rollback();
            LOG_SQL("Transaction rolled back successfully");
        }
        catch (const std::exception& rbEx)
        {
            LOG_ERROR("Rollback failed: {}", rbEx.what());
        }
        catch (...)
        {
            LOG_ERROR("Rollback failed with unknown exception");
        }

        return false;
    }
}

void ContractorVisitManager::NormalizeWorkerChanges(std::unordered_set<std::uint64_t>& workersToAdd, std::unordered_set<std::uint64_t>& workersToRemove)
{
    for (auto it = workersToAdd.begin(); it != workersToAdd.end();)
    {
        if (workersToRemove.contains(*it))
            it = workersToAdd.erase(it);
        else
            ++it;
    }
}
