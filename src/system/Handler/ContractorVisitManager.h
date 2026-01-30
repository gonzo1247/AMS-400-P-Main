#pragma once
#include <unordered_set>

#include "ConnectionGuard.h"
#include "DatabaseDefines.h"

struct ContractorVisitWorkerInfo;

class ContractorVisitManager
{
public:
    ContractorVisitManager();
    ~ContractorVisitManager() = default;

    // Dashboard Data Loaders
    void LoadContractorVisitData();

    std::vector<ContractorVisitInformation>& GetContractorVisits() { return _contractorVisit; }

    // Contract Visit UI (Admin)
    std::vector<ContractorVisitInformation> GetActiveContractorVisits();
    ContractorVisitInformation GetContractorVisitByID(std::uint64_t visitID);
    ContractorVisitInformation LoadContractorVisitByID(std::uint64_t visitID);

    std::vector<ContractorVisitWorkerInfo> GetWorkersForVisit(std::uint64_t visitID);

    std::uint64_t AddNewWorker(const ContractorVisitWorkerInfo& workerInfo, std::uint64_t companyID = 0);

    bool UpdateVisit(const ContractorVisitInformation& visitInfo, std::unordered_set<std::uint64_t> workerToAdd,
                     std::unordered_set<std::uint64_t> workerToRemove);
    bool AddWorkerToVisit(database::DatabaseConnection& connection, const std::unordered_set<std::uint64_t>& workerToAdd, std::uint64_t visitID);
    bool RemoveWorkerFromVisit(database::DatabaseConnection& connection, const std::unordered_set<std::uint64_t>& workerToRemove, std::uint64_t visitID);
    bool AddNewVisit(const ContractorVisitInformation& visitInfo, std::unordered_set<std::uint64_t> workerToAdd, std::unordered_set<std::uint64_t> workerToRemove);

private:
    void NormalizeWorkerChanges(std::unordered_set<std::uint64_t>& workersToAdd, std::unordered_set<std::uint64_t>& workersToRemove);

    std::vector<ContractorVisitInformation> _contractorVisit;
    std::vector<ContractorVisitInformation> _activeVisit;
    std::unordered_map<std::uint64_t, ContractorVisitInformation> _visitByID;
};
