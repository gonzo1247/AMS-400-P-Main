#include "CostUnitDataHandler.h"

#include <ranges>

#include "ConnectionGuard.h"
#include "DatabaseTypes.h"
#include "pch.h"

void CostUnitDataHandler::LoadCostUnit()
{
    // Implementation to load cost unit data from the database

    // SELECT ID, Locale, CostUnitID, CostUnitName, Place, Barcode FROM company_cost_unit
    ConnectionGuardIMS connection(database::ConnectionType::Sync);
    auto select = connection->GetPreparedStatement(database::Implementation::IMSPreparedStatement::DB_CCU_SELECT_ALL_COST_UNITS);

    auto result = connection->ExecutePreparedSelect(*select);

    if (!result.IsValid())
        return;

    costUnitMap.clear();

    while (result.Next())
    {
        Field* fields = result.Fetch();
        CostUnitInformation costUnit;
        costUnit.id = fields[0].GetUInt16();
        std::string locale = fields[1].GetString();
        costUnit.costUnitID = fields[2].GetUInt32();
        costUnit.costUnitNames[locale] = fields[3].GetString();
        costUnit.place = fields[4].GetString();
        costUnit.barcode = fields[5].GetString();

        costUnitMap[costUnit.id] = costUnit;
    }

    // Create the first entry with zero number
    CostUnitInformation info;
    info.id = 0;
    info.costUnitID = 0;
    info.costUnitNames[GetSettings().getLanguage()] = "No Cost Unit";
    info.place = GetCompanyLocationsString(GetSettings().getCompanyLocation());
    info.barcode = "";
    costUnitMap[info.costUnitID] = info;

}

void CostUnitDataHandler::FillComboBoxWithData(QComboBox* comboBox, const std::string& locale, const std::string& place)
{
    comboBox->clear();

    if (costUnitMap.empty())
        return;

    std::vector<std::pair<std::uint32_t, QString>> items;
    items.reserve(costUnitMap.size());

    // Build vector of (ID -> Name)
    for (const auto& [id, info] : costUnitMap)
    {
        // Filter by place if required
        if (!place.empty() && info.place != place)
            continue;

        // Lookup localized name
        auto it = info.costUnitNames.find(locale);
        if (it != info.costUnitNames.end())
        {
            QString combined = QStringLiteral("%1 - %2").arg(info.costUnitID).arg(QString::fromUtf8(it->second));
            items.emplace_back(id, combined);
        }
    }

    // Sort by CostUnitID (ascending)
    std::ranges::sort(items, [](const auto& a, const auto& b) { return a.first < b.first; });

    // Fill combobox
    for (const auto& [id, name] : items)
    {
        comboBox->addItem(name, QVariant::fromValue(static_cast<int>(id)));
    }
}

std::string CostUnitDataHandler::GetCostUnitNameById(const std::uint32_t id)
{
    // Find cost unit by ID
    for (const auto& [key, info] : costUnitMap)
    {
        if (info.costUnitID == id)
        {
            auto it = info.costUnitNames.find(GetSettings().getLanguage());
            if (it != info.costUnitNames.end())
            {
                std::string combine = Util::ConvertUint32ToString(info.costUnitID) + " - " + it->second;
                return combine;
            }
        }
    }

    return{};
}

std::string CostUnitDataHandler::GetCostUnitNameByInternalId(std::uint32_t id)
{
    // Find cost unit by InternalID
    for (const auto& info : costUnitMap | std::views::values)
    {
        if (info.id == id)
        {
            auto it = info.costUnitNames.find(GetSettings().getLanguage());
            if (it != info.costUnitNames.end())
            {
                std::string combine = Util::ConvertUint32ToString(info.costUnitID) + " - " + it->second;
                return combine;
            }
        }
    }

    return {};
}

std::unordered_map<std::uint32_t /*ID*/, std::string /*combined Name*/> CostUnitDataHandler::GetCostUnitNameMap(const std::string& locale, const std::string& place)
{
    std::unordered_map<std::uint32_t, std::string> result;
    result.reserve(costUnitMap.size());  // small speed-up

    for (const auto& [id, info] : costUnitMap)
    {
        // Optional: filter by place if required
        if (!place.empty() && info.place != place)
            continue;

        // Lookup name for locale
        auto it = info.costUnitNames.find(locale);
        if (it != info.costUnitNames.end())
        {
            std::string combine = Util::ConvertUint32ToString(info.costUnitID) + " - " + it->second;
            result.emplace(id, combine);
        }
    }

    return result;
}

std::unordered_map<std::uint16_t, CostUnitInformation> CostUnitDataHandler::GetCostUnitMapForLocation() const
{    
    std::unordered_map<std::uint16_t, CostUnitInformation> locationMap;

    for (const auto& [id, info] : costUnitMap)
    {
        if (info.place == GetCompanyLocationsString(GetSettings().getCompanyLocation()))
        {
            locationMap.emplace(id, info);
        }
    }

    return locationMap;
}

void CostUnitDataHandler::Initialize()
{
    if (_dataAlreadyLoaded)
        return;

    LoadCostUnit();
    _dataAlreadyLoaded = true;
}
