#pragma once
#include "DatabaseDefines.h"

class CostUnitDataHandler
{
public:
    // Singleton accessor
    static CostUnitDataHandler& instance()
    {
        static CostUnitDataHandler instance;
        return instance;
    }

    // Delete copy/move operations
    CostUnitDataHandler(const CostUnitDataHandler&) = delete;
    CostUnitDataHandler& operator=(const CostUnitDataHandler&) = delete;
    CostUnitDataHandler(CostUnitDataHandler&&) = delete;
    CostUnitDataHandler& operator=(CostUnitDataHandler&&) = delete;

public:
    CostUnitDataHandler() = default;
    ~CostUnitDataHandler() = default;

    void Initialize();
    void LoadCostUnit();

    void FillComboBoxWithData(QComboBox* comboBox, const std::string& locale, const std::string& place);
    std::string GetCostUnitNameById(std::uint32_t id);
    std::string GetCostUnitNameByInternalId(std::uint32_t id);

    std::unordered_map<std::uint32_t/*ID*/, std::string /*combined Name*/> GetCostUnitNameMap(const std::string& locale, const std::string& place);
    std::unordered_map<std::uint16_t, CostUnitInformation> GetCostUnitMap() const { return costUnitMap; }
    std::unordered_map<std::uint16_t, CostUnitInformation> GetCostUnitMapForLocation() const;


private:
    std::unordered_map<std::uint16_t, CostUnitInformation> costUnitMap;
    bool _dataAlreadyLoaded =  false;
};

