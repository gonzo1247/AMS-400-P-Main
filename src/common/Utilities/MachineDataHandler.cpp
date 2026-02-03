#include "pch.h"
#include "MachineDataHandler.h"

#include <future>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <unordered_map>

#include "ConnectionGuard.h"
#include "LoggerDefines.h"
#include "Logger.h"

template <typename Fn>
void MachineDataHandler::RunWithRetry(const char* name, Fn&& fn)
{
    constexpr int maxAttempts = 3;
    constexpr int backoffMs[maxAttempts] = {300, 800, 1500};

    std::string lastError;

    for (int attempt = 1; attempt <= maxAttempts; ++attempt)
    {
        try
        {
            fn();
            return;
        }
        catch (const std::exception& ex)
        {
            lastError = ex.what();

            if (attempt < maxAttempts)
                std::this_thread::sleep_for(std::chrono::milliseconds(backoffMs[attempt - 1]));
        }
        catch (...)
        {
            lastError = "unknown exception";

            if (attempt < maxAttempts)
                std::this_thread::sleep_for(std::chrono::milliseconds(backoffMs[attempt - 1]));
        }
    }

    // All retries failed -> log once
    {
        std::scoped_lock lock(_errorMutex);
        _lastError = std::string(name) + " failed after " + std::to_string(maxAttempts) + " attempts: " + lastError;
    }

    _initFailed.store(true, std::memory_order_release);

    // central logging (example)
    LOG_SQL("MachineDataHandler: {}", _lastError);
}

std::string MachineDataHandler::GetMachineLineNameByID(std::uint32_t internalID)
{
    std::shared_lock lock(_dataMutex);
    auto it = _machineLineById.find(internalID);
    if (it == _machineLineById.end())
        return {};

    return it->second.name;
}

std::string MachineDataHandler::GetMachineManufacturerNameByID(std::uint32_t internalID)
{
    std::shared_lock lock(_dataMutex);
    auto it = _machineManufacturerNameById.find(internalID);
    if (it == _machineManufacturerNameById.end())
        return {};

    return it->second;
}

std::string MachineDataHandler::GetMachineTypeByID(std::uint32_t internalID)
{
    std::shared_lock lock(_dataMutex);
    auto it = _machineTypeNameById.find(internalID);
    if (it == _machineTypeNameById.end())
        return {};

    return it->second;
}

std::string MachineDataHandler::GetFacilityRoomByID(std::uint32_t internalID)
{
    std::shared_lock lock(_dataMutex);
    auto it = _facilityRoomById.find(internalID);
    if (it == _facilityRoomById.end())
        return {};

    return it->second.room_code + " " + it->second.room_name;
}

const std::vector<std::uint32_t>* MachineDataHandler::GetFacilityRoomIDsByLocation(CompanyLocations cl) const
{
    std::shared_lock lock(_dataMutex);
    auto it = _facilityRoomIdsByLocation.find(cl);
    if (it == _facilityRoomIdsByLocation.end())
        return nullptr;

    return &it->second;
}

const std::vector<std::uint32_t>* MachineDataHandler::GetMachineLineIDsByLocation(CompanyLocations cl) const
{
    std::shared_lock lock(_dataMutex);
    auto it = _machineLineIdsByLocation.find(cl);
    if (it == _machineLineIdsByLocation.end())
        return nullptr;

    return &it->second;
}

void MachineDataHandler::Initialize()
{
    bool expected = false;
    if (!_initStarted.compare_exchange_strong(expected, true))
        return;

    _initFailed.store(false, std::memory_order_release);
    _fullyLoaded.store(false, std::memory_order_release);

    _f1 = std::async(std::launch::async, [this]() { RunWithRetry("LoadFacilityRoom", [this]() { LoadFacilityRoom(); }); });

    _f2 = std::async(std::launch::async, [this]() { RunWithRetry("LoadMachineLine", [this]() { LoadMachineLine(); }); });

    _f3 = std::async(std::launch::async, [this]() { RunWithRetry("LoadMachineManufacturer", [this]() { LoadMachineManufacturer(); }); });

    _f4 = std::async(std::launch::async, [this]() { RunWithRetry("LoadMachineType", [this]() { LoadMachineType(); }); });

    _joinThread = std::jthread([this]() { WaitUntilReady(); });
}

void MachineDataHandler::LoadMachineLine()
{
    ConnectionGuardAMS connection(ConnectionType::Sync);

    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_ML_SELECT_ALL_LINES_OVER_LOCATIONS);
    auto result = connection->ExecutePreparedSelect(*stmt);

    if (!result.IsValid())
        return;

    std::unordered_map<std::uint32_t, MachineLineData> byId;
    std::unordered_map<CompanyLocations, std::vector<std::uint32_t>> idsByLoc;

    while (result.Next())
    {
        Field* fields = result.Fetch();

        MachineLineData lineData;
        lineData.id = fields[0].GetUInt32();
        lineData.name = fields[1].GetString();
        lineData.is_deleted = fields[2].GetBool();
        if (!fields[3].IsNull())
            lineData.deleted_at = fields[3].GetDateTime();
        else
            lineData.deleted_at = SystemTimePoint{};

        const CompanyLocations location = static_cast<CompanyLocations>(fields[4].GetUInt32());

        idsByLoc[location].push_back(lineData.id);
        byId.emplace(lineData.id, std::move(lineData));
    }

    {
        std::unique_lock lock(_dataMutex);
        _machineLineById = std::move(byId);
        _machineLineIdsByLocation = std::move(idsByLoc);
    }
}

void MachineDataHandler::LoadMachineManufacturer()
{
    ConnectionGuardAMS connection(ConnectionType::Sync);

    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_MM_SELECT_ALL_MANUFACTURERS);
    stmt->SetBool(0, false);

    auto result = connection->ExecutePreparedSelect(*stmt);

    if (!result.IsValid())
        return;

    std::vector<MachineManufacturerData> data;
    std::unordered_map<std::uint32_t, std::string> nameById;

    while (result.Next())
    {
        Field* fields = result.Fetch();

        MachineManufacturerData m;
        m.id = fields[0].GetUInt32();
        m.name = fields[1].GetString();
        m.is_deleted = fields[2].GetBool();
        if (!fields[3].IsNull())
            m.deleted_at = fields[3].GetDateTime();
        else
            m.deleted_at = SystemTimePoint{};

        nameById.emplace(m.id, m.name);
        data.push_back(std::move(m));
    }

    {
        std::unique_lock lock(_dataMutex);
        _machineManufacturerData = std::move(data);
        _machineManufacturerNameById = std::move(nameById);
    }
}

void MachineDataHandler::LoadMachineType()
{
    ConnectionGuardAMS connection(ConnectionType::Sync);

    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_MT_SELECT_ALL_TYPES);
    stmt->SetBool(0, false);

    auto result = connection->ExecutePreparedSelect(*stmt);

    if (!result.IsValid())
        return;

    std::vector<MachineTypeData> data;
    std::unordered_map<std::uint32_t, std::string> nameById;

    while (result.Next())
    {
        Field* fields = result.Fetch();

        MachineTypeData t;
        t.id = fields[0].GetUInt32();
        t.type = fields[1].GetString();
        t.is_deleted = fields[2].GetBool();
        if (!fields[3].IsNull())
            t.deleted_at = fields[3].GetDateTime();
        else
            t.deleted_at = SystemTimePoint{};

        nameById.emplace(t.id, t.type);
        data.push_back(std::move(t));
    }

    {
        std::unique_lock lock(_dataMutex);
        _machineTypeData = std::move(data);
        _machineTypeNameById = std::move(nameById);
    }
}

void MachineDataHandler::LoadFacilityRoom()
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_FR_SELECT_ALL_ROOMS);

    auto result = connection->ExecutePreparedSelect(*stmt);

    if (!result.IsValid())
        return;

    std::unordered_map<std::uint32_t, FacilityRoomData> byId;
    std::unordered_map<CompanyLocations, std::vector<std::uint32_t>> idsByLoc;

    while (result.Next())
    {
        Field* fields = result.Fetch();

        FacilityRoomData roomData;
        roomData.id = fields[0].GetUInt32();
        roomData.room_code = fields[1].GetString();
        roomData.room_name = fields[2].GetString();
        roomData.is_deleted = fields[3].GetBool();
        if (!fields[4].IsNull())
            roomData.deleted_at = fields[4].GetDateTime();
        else
            roomData.deleted_at = SystemTimePoint{};

        const CompanyLocations location = static_cast<CompanyLocations>(fields[5].GetUInt32());

        idsByLoc[location].push_back(roomData.id);
        byId.emplace(roomData.id, std::move(roomData));
    }

    {
        std::unique_lock lock(_dataMutex);
        _facilityRoomById = std::move(byId);
        _facilityRoomIdsByLocation = std::move(idsByLoc);
    }
}

void MachineDataHandler::WaitUntilReady()
{
    try
    {
        _f1.get();
        _f2.get();
        _f3.get();
        _f4.get();
    }
    catch (const std::exception& ex)
    {
        {
            std::scoped_lock lock(_errorMutex);
            _lastError = std::string("WaitUntilReady failed: ") + ex.what();
        }
        _initFailed.store(true, std::memory_order_release);
    }
    catch (...)
    {
        {
            std::scoped_lock lock(_errorMutex);
            _lastError = "WaitUntilReady failed: unknown exception";
        }
        _initFailed.store(true, std::memory_order_release);
    }

    _fullyLoaded.store(true, std::memory_order_release);
}

bool MachineDataHandler::IsReady() const
{
    return _fullyLoaded.load(std::memory_order_acquire) && !_initFailed.load(std::memory_order_acquire);
}

bool MachineDataHandler::HasFailed() const
{
    return _fullyLoaded.load(std::memory_order_acquire) && _initFailed.load(std::memory_order_acquire);
}

std::string MachineDataHandler::GetLastError() const
{
    std::scoped_lock lock(_errorMutex);
    return _lastError;
}
