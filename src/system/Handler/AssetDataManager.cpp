#include "pch.h"
#include "AssetDataManager.h"

#include "AMSDatabase.h"
#include "ConnectionGuard.h"

#include <QCompleter>
#include <algorithm>
#include <ranges>

AssetDataManager::AssetDataManager() {}

std::vector<LookupTableModel::Row> AssetDataManager::LoadMachineLines(std::uint32_t location, bool includeDeleted)
{
    std::vector<LookupTableModel::Row> machineLines;

    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_ML_SELECT_ALL_LINES);
    stmt->SetBool(0, includeDeleted);
    stmt->SetUInt(1, location);

    // Load machine lines from the database
    auto result = connection->ExecutePreparedSelect(*stmt);

    if (!result.IsValid())
        return machineLines;

    while (result.Next())
    {
        LookupTableModel::Row line;
        Field* fields = result.Fetch();

        line.id = fields[0].GetUInt32();
        line.name = fields[1].GetString();
        line.isDeleted = fields[2].GetBool();

        if (!fields[3].IsNull())
            line.deletedAt = Util::ConvertToQDateTime(fields[3].GetDateTime());
        else
            line.deletedAt = QDateTime();

        machineLines.push_back(std::move(line));
    }

    return machineLines;
}

std::uint32_t AssetDataManager::AddMachineLine(std::uint32_t location, const std::string& name)
{
    std::uint32_t newId = 0;

    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_ML_INSERT_NEW_LINE);
    stmt->SetString(0, name);
    stmt->SetUInt(1, location);

    // Execute the insert statement
    if (connection->ExecutePreparedInsert(*stmt))
        newId = static_cast<std::uint32_t>(connection->GetLastInsertId());

    return newId;
}

bool AssetDataManager::UpdateMachineLineName(std::uint32_t id, const std::string& newName)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_ML_UPDATE_LINE_BY_ID);
    stmt->SetString(0, newName);
    stmt->SetUInt(1, id);
    return connection->ExecutePreparedUpdate(*stmt);
}

bool AssetDataManager::SoftDeleteMachineLine(std::uint32_t id)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_ML_DELETE_LINE_BY_ID);
    stmt->SetCurrentDate(0);
    stmt->SetUInt(1, id);
    return connection->ExecutePreparedUpdate(*stmt);
}

bool AssetDataManager::RestoreMachineLine(std::uint32_t id)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_ML_UPDATE_RESTORE_LINE_BY_ID);
    stmt->SetUInt(0, id);
    return connection->ExecutePreparedUpdate(*stmt);
}

bool AssetDataManager::CanDeleteMachineLine(std::uint32_t id)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_ML_SELECT_CAN_DELETE_LINE_BY_ID);
    stmt->SetUInt(0, id);
    auto result = connection->ExecutePreparedSelect(*stmt);
    return !result.IsValid() || !result.Next();
}

QString AssetDataManager::GetMachineLineDeleteBlockReason(std::uint32_t /*id*/)
{
    return TranslateText::translate("AssetDataManager", "Block Reason 0x03AD5");
}

std::vector<LookupTableModel::Row> AssetDataManager::LoadMachineTypes(bool includeDeleted)
{
    std::vector<LookupTableModel::Row> result;

    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_MT_SELECT_ALL_TYPES);
    stmt->SetUInt(0, includeDeleted);
    auto queryResult = connection->ExecutePreparedSelect(*stmt);

    if (!queryResult.IsValid())
        return result;

    while (queryResult.Next())
    {
        LookupTableModel::Row type;
        Field* fields = queryResult.Fetch();

        type.id = fields[0].GetUInt32();
        type.name = fields[1].GetString();
        type.isDeleted = fields[2].GetBool();
        if (!fields[3].IsNull())
            type.deletedAt = Util::ConvertToQDateTime(fields[3].GetDateTime());
        else
            type.deletedAt = QDateTime();
        result.push_back(std::move(type));
    }

    return result;
}

std::uint32_t AssetDataManager::AddMachineType(const std::string& name)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_MT_INSERT_NEW_TYPE);
    stmt->SetString(0, name);
    if (connection->ExecutePreparedInsert(*stmt))
    {
        return static_cast<std::uint32_t>(connection->GetLastInsertId());
    }

    return 0;
}

bool AssetDataManager::UpdateMachineTypeName(std::uint32_t id, const std::string& newName)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_MT_UPDATE_TYPE_BY_ID);
    stmt->SetString(0, newName);
    stmt->SetUInt(1, id);
    return connection->ExecutePreparedUpdate(*stmt);
}

bool AssetDataManager::SoftDeleteMachineType(std::uint32_t id)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_MT_DELETE_TYPE_BY_ID);
    stmt->SetCurrentDate(0);
    stmt->SetUInt(1, id);
    return connection->ExecutePreparedUpdate(*stmt);
}

bool AssetDataManager::RestoreMachineType(std::uint32_t id)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_MT_UPDATE_RESTORE_TYPE_BY_ID);
    stmt->SetUInt(0, id);
    return connection->ExecutePreparedUpdate(*stmt);
}

bool AssetDataManager::CanDeleteMachineType(std::uint32_t id)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_MT_SELECT_CAN_DELETE_TYPE_BY_ID);
    stmt->SetUInt(0, id);
    auto result = connection->ExecutePreparedSelect(*stmt);
    return !result.IsValid() || !result.Next();
}

QString AssetDataManager::GetMachineTypeDeleteBlockReason(std::uint32_t /*id*/)
{
    return TranslateText::translate("AssetDataManager", "Block Reason 0x03AD6");
}

std::vector<LookupTableModel::Row> AssetDataManager::LoadMachineManufacturers(bool includeDeleted)
{
    std::vector<LookupTableModel::Row> result;

    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_MM_SELECT_ALL_MANUFACTURERS);
    stmt->SetUInt(0, includeDeleted);

    auto queryResult = connection->ExecutePreparedSelect(*stmt);

    if (!queryResult.IsValid())
        return result;

    while (queryResult.Next())
    {
        LookupTableModel::Row manufacturer;
        Field* fields = queryResult.Fetch();
        manufacturer.id = fields[0].GetUInt32();
        manufacturer.name = fields[1].GetString();
        manufacturer.isDeleted = fields[2].GetBool();
        if (!fields[3].IsNull())
            manufacturer.deletedAt = Util::ConvertToQDateTime(fields[3].GetDateTime());
        else
            manufacturer.deletedAt = QDateTime();
        result.push_back(std::move(manufacturer));
    }

    return result;
}

std::uint32_t AssetDataManager::AddMachineManufacturer(const std::string& name)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_MM_INSERT_NEW_MANUFACTURER);
    stmt->SetString(0, name);
    if (connection->ExecutePreparedInsert(*stmt))
    {
        return static_cast<std::uint32_t>(connection->GetLastInsertId());
    }

    return 0;
}

bool AssetDataManager::UpdateMachineManufacturerName(std::uint32_t id, const std::string& newName)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_MM_UPDATE_MANUFACTURER_BY_ID);
    stmt->SetString(0, newName);
    stmt->SetUInt(1, id);
    return connection->ExecutePreparedUpdate(*stmt);
}

bool AssetDataManager::SoftDeleteMachineManufacturer(std::uint32_t id)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_MM_DELETE_MANUFACTURER_BY_ID);
    stmt->SetCurrentDate(0);
    stmt->SetUInt(1, id);
    return connection->ExecutePreparedUpdate(*stmt);
}

bool AssetDataManager::RestoreMachineManufacturer(std::uint32_t id)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_MM_UPDATE_RESTORE_MANUFACTURER_BY_ID);
    stmt->SetUInt(0, id);
    return connection->ExecutePreparedUpdate(*stmt);
}

bool AssetDataManager::CanDeleteMachineManufacturer(std::uint32_t id)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_MM_SELECT_CAN_DELETE_MANUFACTURER_BY_ID);
    stmt->SetUInt(0, id);
    auto result = connection->ExecutePreparedSelect(*stmt);
    return !result.IsValid() || !result.Next();
}

QString AssetDataManager::GetMachineManufacturerDeleteBlockReason(std::uint32_t /*id*/)
{
    return TranslateText::translate("AssetDataManager", "Block Reason 0x03AD7");
}

std::vector<RoomTableModel::Row> AssetDataManager::LoadRooms(std::uint32_t cLoc, bool showDeleted)
{
    // SELECT ID, room_code, room_name, is_deleted, deleted_at FROM facility_room WHERE companyLocation = ? AND is_deleted = ?

    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_FR_SELECT_ROOMS_BY_LOCATION);
    stmt->SetUInt(0, cLoc);
    stmt->SetBool(1, showDeleted);
    auto queryResult = connection->ExecutePreparedSelect(*stmt);

    if (!queryResult.IsValid())
        return {};

    std::vector<RoomTableModel::Row> rooms;

    while (queryResult.Next())
    {
        RoomTableModel::Row room;
        Field* fields = queryResult.Fetch();
        room.id = fields[0].GetUInt32();
        room.code = fields[1].GetString();
        room.name = fields[2].GetString();
        room.isDeleted = fields[3].GetBool();
        if (!fields[4].IsNull())
            room.deletedAt = Util::ConvertToQDateTime(fields[4].GetDateTime());
        else
            room.deletedAt = QDateTime();
        rooms.push_back(std::move(room));
    }

    return rooms;

}

std::uint32_t AssetDataManager::AddRoom(std::uint32_t cLoc, const std::string& code, const std::string& name)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_FR_INSERT_NEW_ROOM);
    stmt->SetString(0, code);
    stmt->SetString(1, name);
    stmt->SetUInt(2, cLoc);
    if (connection->ExecutePreparedInsert(*stmt))
    {
        return static_cast<std::uint32_t>(connection->GetLastInsertId());
    }

    return 0;
}

bool AssetDataManager::UpdateRoom(std::uint32_t id, const std::string& code, const std::string& name)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_FR_UPDATE_ROOM_BY_ID);
    stmt->SetString(0, code);
    stmt->SetString(1, name);
    stmt->SetUInt(2, id);
    return connection->ExecutePreparedUpdate(*stmt);
}

bool AssetDataManager::SoftDeleteRoom(std::uint32_t id)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_FR_DELETE_ROOM_BY_ID);
    stmt->SetCurrentDate(0);
    stmt->SetUInt(1, id);
    return connection->ExecutePreparedUpdate(*stmt);
}

bool AssetDataManager::RestoreRoom(std::uint32_t id)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_FR_UPDATE_RESTORE_ROOM_BY_ID);
    stmt->SetUInt(0, id);
    return connection->ExecutePreparedUpdate(*stmt);
}

bool AssetDataManager::CanDeleteRoom(std::uint32_t id)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_FR_SELECT_CAN_DELETE_ROOM_BY_ID);
    stmt->SetUInt(0, id);
    auto result = connection->ExecutePreparedSelect(*stmt);
    return !result.IsValid() || !result.Next();
}

QString AssetDataManager::GetRoomDeleteBlockReason(std::uint32_t /*id*/)
{
    return TranslateText::translate("AssetDataManager", "Block Reason 0x03AD8");
}

std::vector<AssetDataManager::ComboEntry> AssetDataManager::GetRoomsForLocation(CompanyLocations cl)
{
    const auto rooms = LoadRoomData(cl);

    std::vector<ComboEntry> out;
    out.reserve(rooms.size());

    for (const auto& r : rooms)
    {
        out.push_back({r.id, r.name});
    }

    return out;
}

std::vector<AssetDataManager::ComboEntry> AssetDataManager::GetLineForBox(CompanyLocations cl /*= CompanyLocations::CL_BBG*/)
{
    auto data = LoadLineData(cl);

    std::vector<ComboEntry> out;
    out.reserve(data.size());

    for (const auto& r : data)
    {
        out.push_back({r.id, r.name});
    }

    return out;
}

std::vector<AssetDataManager::ComboEntry> AssetDataManager::GetManufacturerForBox()
{
    auto data = LoadManufacturerData();

    std::vector<ComboEntry> out;
    out.reserve(data.size());
    
    for (const auto& r : data)
    {
        out.push_back({r.id, r.name});
    }

    return out;
}

std::vector<AssetDataManager::ComboEntry> AssetDataManager::GetTypeForBox()
{
    auto data = LoadManufacturerData();
    std::vector<ComboEntry> out;
    out.reserve(data.size());

    for (const auto& r : data)
    {
        out.push_back({r.id, r.name});
    }

    return out;
}

AssetDataManager::AddMachineResult AssetDataManager::AddNewMachine(const MachineInformation& machineInfo)
{
    if (machineInfo.empty())
        return {.id = 0, .ok = false };

    // INSERT INTO machine_line (CostUnitID, MachineTypeID, LineID, ManufacturerID, MachineName,
    // MachineNumber, ManufacturerMachineNumber, RoomNumber, MoreInformation, locationID) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    AddMachineResult result{};

    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_ML_INSERT_NEW_MACHINE);

    stmt->SetUInt(0, machineInfo.CostUnitID);
    stmt->SetUInt(1, machineInfo.MachineTypeID);
    stmt->SetUInt(2, machineInfo.LineID);
    stmt->SetUInt(3, machineInfo.ManufacturerID);
    stmt->SetString(4, machineInfo.MachineName);
    stmt->SetString(5, machineInfo.MachineNumber);
    stmt->SetString(6, machineInfo.ManufacturerMachineNumber);
    stmt->SetUInt(7, machineInfo.RoomID);
    stmt->SetString(8, machineInfo.MoreInformation);
    stmt->SetUInt(9, machineInfo.locationID);

    result.ok = connection->ExecutePreparedInsert(*stmt);
    result.id = connection->GetLastInsertId();

    return result;
}

AssetDataManager::SqlUpdate AssetDataManager::BuildMachineUpdateSql(int machineId, const MachineInformation& info,
                                                                    const ChangeTracker<MachineField>& tracker)
{
    SqlUpdate out;

    const auto dirty = tracker.GetDirtyFields();
    if (dirty.isEmpty())
        return out;

    std::string setPart;
    std::vector<QVariant> params;

    auto add = [&](MachineField field, const QVariant& value)
    {
        const char* col = MachineFieldToColumn(field);
        if (col[0] == '\0')
            return;

        if (!setPart.empty())
            setPart += ", ";

        setPart += col;
        setPart += " = ?";

        params.push_back(value);
    };

    for (const auto& field : dirty)
    {
        switch (field)
        {
            case MachineField::Name:
                add(field, QString::fromStdString(info.MachineName));
                break;
            case MachineField::Number:
                add(field, QString::fromStdString(info.MachineNumber));
                break;
            case MachineField::ManufacturerNumber:
                add(field, QString::fromStdString(info.ManufacturerMachineNumber));
                break;

            case MachineField::CostUnitId:
                add(field, info.CostUnitID);
                break;
            case MachineField::MachineTypeId:
                add(field, info.MachineTypeID);
                break;
            case MachineField::MachineLineId:
                add(field, info.LineID);
                break;
            case MachineField::ManufacturerId:
                add(field, info.ManufacturerID);
                break;

            case MachineField::RoomId:
                add(field, info.RoomID);
                break;
            case MachineField::Location:
                add(field, info.locationID);
                break;
            case MachineField::Info:
                add(field, QString::fromStdString(info.MoreInformation));
                break;

            default:
                break;
        }
    }

    out.sql = "UPDATE machine_list SET " + setPart + " WHERE ID = ?";
    out.params = std::move(params);
    out.params.emplace_back(machineId);

    return out;
}

bool AssetDataManager::UpdateMachineData(const MachineInformation& info, const ChangeTracker<MachineField>& tracker)
{
    auto sqlUpdate = BuildMachineUpdateSql(info.ID, info, tracker);
    if (sqlUpdate.sql.empty())
        return false;

    ConnectionGuardAMS connection(ConnectionType::Sync);

    auto stmt = connection->GetStatementRaw(sqlUpdate.sql);

    for (std::size_t i = 0; i < sqlUpdate.params.size(); ++i)
    {
        stmt->SetQVariant(i, sqlUpdate.params[i]);
    }

    return connection->ExecutePreparedUpdate(*stmt);
}

const char* AssetDataManager::MachineFieldToColumn(MachineField field)
{
    switch (field)
    {
        case MachineField::Name:
            return "MachineName";
        case MachineField::Number:
            return "MachineNumber";
        case MachineField::ManufacturerNumber:
            return "ManufacturerMachineNumber";
        case MachineField::CostUnitId:
            return "CostUnitID";
        case MachineField::MachineTypeId:
            return "MachineTypeID";
        case MachineField::MachineLineId:
            return "LineID";
        case MachineField::ManufacturerId:
            return "ManufacturerID";
        case MachineField::RoomId:
            return "RoomNumber";
        case MachineField::Location:
            return "location";
        case MachineField::Info:
            return "MoreInformation";
        default:
            return "";
    }
}

AssetDataManager::MachineValidationResult AssetDataManager::Validate(const MachineInformation& m) noexcept
{
    // Limits are domain rules
    constexpr std::size_t NAME_MAX = 200;
    constexpr std::size_t NUM_MAX = 50;
    constexpr std::size_t INFO_MAX = 255;

    const std::string name = TrimCopy(m.MachineName);
    if (name.empty())
        return {MachineValidationError::NameEmpty, 0};
    if (name.size() > NAME_MAX)
        return {MachineValidationError::NameTooLong, NAME_MAX};

    const std::string number = TrimCopy(m.MachineNumber);
    if (number.size() > NUM_MAX)
        return {MachineValidationError::NumberTooLong, NUM_MAX};

    const std::string manNum = TrimCopy(m.ManufacturerMachineNumber);
    if (manNum.size() > NUM_MAX)
        return {MachineValidationError::ManufacturerNumberTooLong, NUM_MAX};

    // Info: usually not trimmed; if you want, trim as well
    if (m.MoreInformation.size() > INFO_MAX)
        return {MachineValidationError::InfoTooLong, INFO_MAX};

    return {};
}

std::string AssetDataManager::TrimCopy(const std::string& s)
{
    auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };

    auto begin = std::ranges::find_if_not(s, isSpace);
    auto end = std::ranges::find_if_not(std::ranges::reverse_view(s), isSpace).base();

    if (begin >= end)
        return {};

    return std::string(begin, end);
}

std::vector<AssetDataManager::MachineData> AssetDataManager::LoadRoomData(CompanyLocations cl)
{
    // SELECT ID, room_code, room_name, is_deleted, deleted_at FROM facility_room WHERE companyLocation

    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_FR_SELECT_ROOM_BY_LOCATION_ONLY);
    stmt->SetUInt(0, static_cast<int>(cl));

    auto queryResult = connection->ExecutePreparedSelect(*stmt);

    if (!queryResult.IsValid())
        return {};

    std::vector<MachineData> rooms;

    while (queryResult.Next())
    {
        MachineData room;
        Field* fields = queryResult.Fetch();
        room.id = fields[0].GetUInt32();
        room.code = fields[1].GetString();
        room.name = fields[2].GetString();

        rooms.push_back(std::move(room));
    }

    return rooms;
}

std::vector<AssetDataManager::MachineData> AssetDataManager::LoadLineData(CompanyLocations cl)
{
    std::vector<MachineData> machineLines;

    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_ML_SELECT_ALL_LINES);
    stmt->SetBool(0, false);
    stmt->SetUInt(1, static_cast<int>(cl));

    // Load machine lines from the database
    auto result = connection->ExecutePreparedSelect(*stmt);

    if (!result.IsValid())
        return machineLines;

    while (result.Next())
    {
        MachineData line;
        Field* fields = result.Fetch();

        line.id = fields[0].GetUInt32();
        line.name = fields[1].GetString();

        machineLines.push_back(std::move(line));
    }

    return machineLines;
}

std::vector<AssetDataManager::MachineData> AssetDataManager::LoadTypeData()
{
    std::vector<MachineData> result;

    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_MT_SELECT_ALL_TYPES);
    stmt->SetUInt(0, false);
    auto queryResult = connection->ExecutePreparedSelect(*stmt);

    if (!queryResult.IsValid())
        return result;

    while (queryResult.Next())
    {
        MachineData type;
        Field* fields = queryResult.Fetch();

        type.id = fields[0].GetUInt32();
        type.name = fields[1].GetString();

        result.push_back(std::move(type));
    }

    return result;
}

std::vector<AssetDataManager::MachineData> AssetDataManager::LoadManufacturerData()
{
    std::vector<MachineData> result;

    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_MM_SELECT_ALL_MANUFACTURERS);
    stmt->SetUInt(0, false);

    auto queryResult = connection->ExecutePreparedSelect(*stmt);

    if (!queryResult.IsValid())
        return result;

    while (queryResult.Next())
    {
        MachineData manufacturer;
        Field* fields = queryResult.Fetch();
        manufacturer.id = fields[0].GetUInt32();
        manufacturer.name = fields[1].GetString();

        result.push_back(std::move(manufacturer));
    }

    return result;
}
