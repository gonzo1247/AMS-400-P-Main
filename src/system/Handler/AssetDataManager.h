#pragma once
#include "ChangeTracker.h"
#include "LookupTableModel.h"
#include "RoomTableModel.h"
#include "SharedDefines.h"

enum class MachineField : std::uint8_t;
struct MachineInformation;

class AssetDataManager
{
public:
    struct MachineData
    {
        std::uint32_t id{};
        std::string name{}; // Name or room_name
        std::string code {}; // room_code
    };

    struct AddMachineResult
    {
        std::uint32_t id{};
        bool ok{};
    };

    struct SqlUpdate
    {
        std::string sql;
        std::vector<QVariant> params;
    };

    struct ComboEntry
    {
        std::uint32_t id;
        std::string text;
    };

    enum class MachineValidationError
    {
        None = 0,

        NameEmpty,
        NameTooLong,

        NumberTooLong,
        ManufacturerNumberTooLong,

        InfoTooLong
    };

    struct MachineValidationResult
    {
        MachineValidationError error = MachineValidationError::None;
        std::size_t limit = 0;  // for "TooLong" errors (optional)
    };

    explicit AssetDataManager();

    // Machine Line Data Management
    std::vector<LookupTableModel::Row> LoadMachineLines(std::uint32_t location, bool includeDeleted);
    std::uint32_t AddMachineLine(std::uint32_t location, const std::string& name);
    bool UpdateMachineLineName(std::uint32_t id, const std::string& newName);
    bool SoftDeleteMachineLine(std::uint32_t id);
    bool RestoreMachineLine(std::uint32_t id);
    bool CanDeleteMachineLine(std::uint32_t id);
    QString GetMachineLineDeleteBlockReason(std::uint32_t id);

    // Machine Type Data Management
    std::vector<LookupTableModel::Row> LoadMachineTypes(bool includeDeleted);
    std::uint32_t AddMachineType(const std::string& name);
    bool UpdateMachineTypeName(std::uint32_t id, const std::string& newName);
    bool SoftDeleteMachineType(std::uint32_t id);
    bool RestoreMachineType(std::uint32_t id);
    bool CanDeleteMachineType(std::uint32_t id);
    QString GetMachineTypeDeleteBlockReason(std::uint32_t id);

    // Machine Manufacturer Data Management
    std::vector<LookupTableModel::Row> LoadMachineManufacturers(bool includeDeleted);
    std::uint32_t AddMachineManufacturer(const std::string& name);
    bool UpdateMachineManufacturerName(std::uint32_t id, const std::string& newName);
    bool SoftDeleteMachineManufacturer(std::uint32_t id);
    bool RestoreMachineManufacturer(std::uint32_t id);
    bool CanDeleteMachineManufacturer(std::uint32_t id);
    QString GetMachineManufacturerDeleteBlockReason(std::uint32_t id);

    // facility_room data management
    std::vector<RoomTableModel::Row> LoadRooms(std::uint32_t cLoc, bool showDeleted);
    std::uint32_t AddRoom(std::uint32_t cLoc, const std::string& code, const std::string& name);
    bool UpdateRoom(std::uint32_t id, const std::string& code, const std::string& name);
    bool SoftDeleteRoom(std::uint32_t id);
    bool RestoreRoom(std::uint32_t id);
    bool CanDeleteRoom(std::uint32_t id);
    QString GetRoomDeleteBlockReason(std::uint32_t id);


    // Combobox Handling
    std::vector<ComboEntry> GetRoomsForLocation(CompanyLocations cl);
    std::vector<ComboEntry> GetLineForBox(CompanyLocations cl = CompanyLocations::CL_BBG);
    std::vector<ComboEntry> GetManufacturerForBox();
    std::vector<ComboEntry> GetTypeForBox();

    // Machine List
    AddMachineResult AddNewMachine(const MachineInformation& machineInfo);

    SqlUpdate BuildMachineUpdateSql(int machineId, const MachineInformation& info, const ChangeTracker<MachineField>& tracker);
    bool UpdateMachineData(const MachineInformation& info, const ChangeTracker<MachineField>& tracker);

    static const char* MachineFieldToColumn(MachineField field);

    MachineValidationResult Validate(const MachineInformation& m) noexcept;;

private:
    static std::string TrimCopy(const std::string& s);

    std::vector<MachineData> LoadRoomData(CompanyLocations cl);
    std::vector<MachineData> LoadLineData(CompanyLocations cl);
    std::vector<MachineData> LoadTypeData();
    std::vector<MachineData> LoadManufacturerData();
};
