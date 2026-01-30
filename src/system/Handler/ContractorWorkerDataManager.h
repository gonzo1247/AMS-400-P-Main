#pragma once

struct ContractorWorkerDataDB;

class ContractorWorkerDataManager
{
public:
    explicit ContractorWorkerDataManager();
    ~ContractorWorkerDataManager();

    std::vector<ContractorWorkerDataDB> GetAllWorkers();
    ContractorWorkerDataDB GetWorkerByID(std::uint32_t id);

    bool SaveNewWorker(const ContractorWorkerDataDB& worker);
    bool UpdateWorker(const ContractorWorkerDataDB& worker);
    bool SoftDeleteWorker(std::uint32_t workerID);

private:
    std::vector<ContractorWorkerDataDB> _workers;
    std::unordered_map<std::uint32_t /*id*/, ContractorWorkerDataDB> _workersByID;
};
