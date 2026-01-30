#include "pch.h"
#include "CompanyContactDataManager.h"

#include "ConnectionGuard.h"


std::vector<CompanyContactEntry> CompanyContactDataManager::LoadAll(bool includeDeleted)
{
    std::vector<CompanyContactEntry> out;

    //        0     1           2             3              4            5           6        7        8
    // SELECT ID, CompanyID, CompanyName, ContactPerson, MailAddress, PhoneNumber, Address, Street, HouseNumber, "
    //      9       10      11      12                  13              14          15        16        17
    //PostalCode, City, Country, ResponsibleFor, CustomerNumber, MoreInformation, Website, highlight, IsDeleted FROM company_contact_data WHERE (? = 1 OR IsDeleted = 0)


    ConnectionGuardIMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(IMSPreparedStatement::DB_CCD_SELECT_ALL_COMPANY_CONTACTS);

    stmt->SetUInt(0, includeDeleted ? 1U : 0U);

    auto rs = connection->ExecutePreparedSelect(*stmt);

    if (!rs.IsValid())
    {
        LOG_ERROR("Failed to load company contact data entries from database.");
        return out;
    }

    while (rs.Next())
    {
        CompanyContactEntry e{};

        Field* fields = rs.Fetch();

        e.id = fields[0].GetUInt32();
        e.companyId = fields[1].GetUInt32();
        e.companyName = fields[2].GetQString();
        e.contactPerson = fields[3].GetQString();
        e.mailAddress = fields[4].GetQString();
        e.phoneNumber = fields[5].GetQString();
        e.address = fields[6].GetQString();
        e.street = fields[7].GetQString();
        e.houseNumber = fields[8].GetQString();
        e.postalCode = fields[9].GetQString();
        e.city = fields[10].GetQString();
        e.country = fields[11].GetQString();
        e.responsibleFor = fields[12].GetQString();
        e.customerNumber = fields[13].GetQString();
        e.moreInformation = fields[14].GetQString();
        e.website = fields[15].GetQString();
        e.highlight = fields[16].GetUInt8();
        e.isDeleted = fields[17].GetUInt8();

        out.emplace_back(std::move(e));
    }

    return out;
}
