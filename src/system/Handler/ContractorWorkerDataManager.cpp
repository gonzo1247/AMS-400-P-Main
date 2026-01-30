#include "pch.h"
#include "ContractorWorkerDataManager.h"

#include "ConnectionGuard.h"
#include "DatabaseDefines.h"

ContractorWorkerDataManager::ContractorWorkerDataManager() {}

ContractorWorkerDataManager::~ContractorWorkerDataManager() {}

std::vector<ContractorWorkerDataDB> ContractorWorkerDataManager::GetAllWorkers()
{
    //        0         1               2                 3          4        5      6      7       8         9          10
    // SELECT id, company_name, company_external_id, first_name, last_name, phone, email, note, is_active, created_at, updated_at FROM contractor_worker

    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_CW_SELECT_ALL_CONTRACTOR_WORKERS);

    auto result = connection->ExecutePreparedSelect(*stmt);

    if (!result.IsValid())
        return {};

    std::vector<ContractorWorkerDataDB> workers;

    while (result.Next())
    {
        ContractorWorkerDataDB worker;

        Field* fields = result.Fetch();
        worker.id = fields[0].GetUInt32();
        worker.companyName = fields[1].GetString();
        worker.companyID = fields[2].GetUInt32();
        worker.firstName = fields[3].GetString();
        worker.lastName = fields[4].GetString();
        worker.phone = fields[5].GetString();
        worker.email = fields[6].GetString();
        worker.note = fields[7].GetString();
        worker.isActive = fields[8].GetBool();
        worker.created_at = fields[9].GetDateTime();

        if (!fields[10].IsNull())
            worker.updated_at = fields[10].GetDateTime();

        _workersByID.emplace(worker.id, worker);

        workers.emplace_back(worker);
        _workers.emplace_back(worker);
    }

    return workers;
}

ContractorWorkerDataDB ContractorWorkerDataManager::GetWorkerByID(std::uint32_t id)
{
    auto itr = _workersByID.find(id);
    if (itr != _workersByID.end())
        return itr->second;
}

bool ContractorWorkerDataManager::SaveNewWorker(const ContractorWorkerDataDB& worker)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_CW_INSERT_NEW_CONTRACTOR_WORKER);
    stmt->SetString(0, worker.companyName);
    stmt->SetUInt(1, worker.companyID);
    stmt->SetString(2, worker.firstName);
    stmt->SetString(3, worker.lastName);
    stmt->SetString(4, worker.phone);
    stmt->SetString(5, worker.email);
    stmt->SetString(6, worker.note);
    stmt->SetBool(7, worker.isActive);

    return connection->ExecutePreparedInsert(*stmt);
}

bool ContractorWorkerDataManager::UpdateWorker(const ContractorWorkerDataDB& worker)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_CW_UPDATE_CONTRACTOR_WORKER_BY_ID);
    stmt->SetString(0, worker.companyName);
    stmt->SetUInt(1, worker.companyID);
    stmt->SetString(2, worker.firstName);
    stmt->SetString(3, worker.lastName);
    stmt->SetString(4, worker.phone);
    stmt->SetString(5, worker.email);
    stmt->SetString(6, worker.note);
    stmt->SetBool(7, worker.isActive);
    stmt->SetUInt(8, worker.id);

    return connection->ExecutePreparedUpdate(*stmt);
}

bool ContractorWorkerDataManager::SoftDeleteWorker(std::uint32_t workerID)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_CW_UPDATE_DELETE_CONTRACTOR_WORKER_BY_ID);
    stmt->SetUInt(0, workerID);

    return connection->ExecutePreparedUpdate(*stmt);
}
