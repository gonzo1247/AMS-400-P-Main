#pragma once
#include "DatabaseDefines.h"

class MachineListManager
{
public:
	MachineListManager() = default;
    ~MachineListManager() = default;

	void LoadMachineListFromDatabase();

    [[nodiscard]] const MachineInformation* GetById(std::uint32_t id) const;
    [[nodiscard]] std::vector<MachineInformation> GetMachineData() const;
    [[nodiscard]] std::unordered_map<std::int32_t, MachineInformation> GetMachineDataMap() const;
    [[nodiscard]] std::vector<MachineInformation> GetMachineDataByCostUnitID(std::uint16_t costUnitID) const;
    void UpdateCachedMachine(const MachineInformation& updated);
    void AddNewMachineToCache(const MachineInformation& newMachine);

private:

    std::vector<MachineInformation> _machineVector;
    std::unordered_map<std::uint32_t, std::size_t> _indexById;
};

