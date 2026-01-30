#include "CompanyLocationHandler.h"

#include "ConnectionGuard.h"
#include "DatabaseTypes.h"
#include "pch.h"

void CompanyLocationHandler::Initialize()
{
    LoadCompanyData();
}

void CompanyLocationHandler::FillComboBoxWithData(QComboBox* comboBox)
{    
    for (const auto& [id, location] : _companyData)
    {
        comboBox->addItem(QString::fromStdString(location.fullName), QVariant(static_cast<int>(id)));
    }
}

std::string CompanyLocationHandler::GetLocationNameById(const std::uint32_t id)
{
    // Lookup location name by ID
    const auto it = _companyData.find(id);
    if (it != _companyData.end())
    {
        return it->second.fullName;
    }
    return "";
}

void CompanyLocationHandler::LoadCompanyData()
{
    // Load company location data from the database

    // SELECT ID, location, FullName FROM company_locations

    ConnectionGuardIMS connection(database::ConnectionType::Sync);

    auto stmt = connection->GetPreparedStatement(IMSPreparedStatement::DB_CL_SELECT_ALL_COMPANY_LOCATIONS);
    auto result = connection->ExecutePreparedSelect(*stmt);

    if (!result.IsValid())
        return;

    while (result.Next())
    {
        auto field = result.Fetch();
        CompanyLocationDatabase locationData;
        locationData.id = field[0].GetUInt32();
        locationData.locationAcronym = field[1].GetString();
        locationData.fullName = field[2].GetString();
        _companyData[locationData.id] = locationData;
    }
}
