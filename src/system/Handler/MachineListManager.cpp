#include "MachineListManager.h"

#include "ConnectionGuard.h"
#include "DatabaseTypes.h"
#include "pch.h"

void MachineListManager::LoadMachineListFromDatabase()
{
    // Implementation to load machine list from the database

    //         0         1          2           3        4              5            6                      7               8               9               10          11          12
    // SELECT ID, CostUnitID, MachineTypeID, LineID, ManufacturerID, MachineName, MachineNumber, ManufacturerMachineNumber, RoomNumber, MoreInformation, location, is_deleted, deleted_at  FROM machine_list
    ConnectionGuardAMS connection(database::ConnectionType::Sync);

    auto statement = connection->GetPreparedStatement(AMSPreparedStatement::DB_ML_SELECT_ALL_MACHINES);
    auto result = connection->ExecutePreparedSelect(*statement);

    if (!result.IsValid())
        return;

    _machineVector.clear();
    _indexById.clear();

    while (result.NextRow())
    {
        Field* fields = result.Fetch();

        MachineInformation machineInfo;
        machineInfo.ID = fields[0].GetInt32();
        machineInfo.CostUnitID = fields[1].IsNull() ? 0 : fields[1].GetUInt16();
        machineInfo.MachineTypeID = fields[2].IsNull() ? 0 : fields[2].GetUInt16();
        machineInfo.LineID = fields[3].IsNull() ? 0 : fields[3].GetUInt16();
        machineInfo.ManufacturerID = fields[4].IsNull() ? 0 : fields[4].GetUInt16();
        machineInfo.MachineName = fields[5].GetString();
        machineInfo.MachineNumber = fields[6].GetString();
        machineInfo.ManufacturerMachineNumber = fields[7].GetString();
        machineInfo.RoomID = fields[8].IsNull() ? 0 : fields[8].GetUInt16();
        machineInfo.MoreInformation = fields[9].GetString();
        machineInfo.locationID = fields[10].IsNull() ? 0u : static_cast<std::uint32_t>(fields[10].GetInt32());
        machineInfo.isActive = !fields[11].GetBool();  // is_deleted
        machineInfo.deleted_at = fields[12].IsNull() ? SystemTimePoint{} : fields[12].GetDateTime();

        _machineVector.push_back(machineInfo);
        _indexById[machineInfo.ID] = _machineVector.size() - 1;
    }
}

const MachineInformation* MachineListManager::GetById(std::uint32_t id) const
{
    if (const auto it = _indexById.find(id); it != _indexById.end())
    {
        return &_machineVector[it->second];
    }

    return nullptr;
}

[[nodiscard]] std::vector<MachineInformation> MachineListManager::GetMachineData() const
{
    return _machineVector;
}

std::unordered_map<std::int32_t, MachineInformation> MachineListManager::GetMachineDataMap() const
{
    std::unordered_map<std::int32_t, MachineInformation> machineMap;

    for (const auto& machine : _machineVector)
        machineMap[machine.ID] = machine;

    return machineMap;
}

std::vector<MachineInformation> MachineListManager::GetMachineDataByCostUnitID(std::uint16_t costUnitID) const
{    
    std::vector<MachineInformation> filteredMachines;

    for (const auto& machine : _machineVector)
    {
        if (machine.CostUnitID == costUnitID)
        {
            filteredMachines.push_back(machine);
        }
    }

    return filteredMachines;
}

void MachineListManager::UpdateCachedMachine(const MachineInformation& updated)
{
    const auto it = _indexById.find(updated.ID);
    if (it == _indexById.end())
        return;

    _machineVector[it->second] = updated;
}

void MachineListManager::AddNewMachineToCache(const MachineInformation& newMachine)
{
    _machineVector.push_back(newMachine);
    _indexById[newMachine.ID] = _machineVector.size() - 1;
}
