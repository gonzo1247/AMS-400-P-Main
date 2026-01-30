#include "pch.h"
#include "ContractorVisitStatusManager.h"

#include "ConnectionGuard.h"

std::vector<ContractorVisitStatusData> ContractorVisitStatusManager::LoadStatus()
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    std::vector<ContractorVisitStatusData> statuses;

    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_CVS_SELECT_ALL_STATUS);

    auto result = connection->ExecutePreparedSelect(*stmt);

    if (!result.IsValid())
        return {};

    while (result.Next())
    {
        Field* fields = result.Fetch();

        ContractorVisitStatusData status;

        status.ID = fields[0].GetUInt16();
        status.status_name = fields[1].GetString();
        status.description = fields[2].GetString();
        statuses.emplace_back(std::move(status));
    }

    return statuses;
}

// Static storage
std::vector<ContractorVisitStatusData> ContractorVisitStatusManager::_cache;
std::unordered_map<std::uint16_t, std::size_t> ContractorVisitStatusManager::_indexById;
bool ContractorVisitStatusManager::_loaded = false;

const std::vector<ContractorVisitStatusData>& ContractorVisitStatusManager::GetAll()
{
    LoadIfNeeded();
    return _cache;
}

const ContractorVisitStatusData* ContractorVisitStatusManager::GetById(std::uint16_t id)
{
    LoadIfNeeded();

    auto it = _indexById.find(id);
    if (it == _indexById.end())
        return nullptr;

    return &_cache[it->second];
}

std::string ContractorVisitStatusManager::GetName(std::uint16_t id)
{
    if (const auto* data = GetById(id))
        return data->status_name;

    return {};
}

std::string ContractorVisitStatusManager::GetDescription(std::uint16_t id)
{
    if (const auto* data = GetById(id))
        return data->description;

    return {};
}

void ContractorVisitStatusManager::Reload()
{
    _loaded = false;
    _cache.clear();
    _indexById.clear();

    LoadIfNeeded();
}

void ContractorVisitStatusManager::LoadIfNeeded()
{
    if (_loaded)
        return;

    _cache = LoadStatus();

    if (_cache.empty())
        return;

    _indexById.clear();
    _indexById.reserve(_cache.size());

    for (std::size_t i = 0; i < _cache.size(); ++i)
        _indexById[_cache[i].ID] = i;

    _loaded = true;
}
