#pragma once
#include <future>
#include <shared_mutex>

struct MachineTypeData;
struct MachineManufacturerData;
struct MachineLineData;
struct FacilityRoomData;
enum class CompanyLocations;

class MachineDataHandler
{
   public:
    // Singleton accessor
    static MachineDataHandler& instance()
    {
        static MachineDataHandler instance;
        return instance;
    }

    // Delete copy/move operations
    MachineDataHandler(const MachineDataHandler&) = delete;
    MachineDataHandler& operator=(const MachineDataHandler&) = delete;
    MachineDataHandler(MachineDataHandler&&) = delete;
    MachineDataHandler& operator=(MachineDataHandler&&) = delete;

    std::string GetMachineLineNameByID(std::uint32_t internalID);
    std::string GetMachineManufacturerNameByID(std::uint32_t internalID);
    std::string GetMachineTypeByID(std::uint32_t internalID);
    std::string GetFacilityRoomByID(std::uint32_t internalID);

    const std::vector<std::uint32_t>* GetFacilityRoomIDsByLocation(CompanyLocations cl) const;
        const
    const std::vector<std::uint32_t>* GetMachineLineIDsByLocation(CompanyLocations cl) const;

   public:
    MachineDataHandler() = default;
    ~MachineDataHandler() = default;

    void Initialize();
    void LoadMachineLine();
    void LoadMachineManufacturer();
    void LoadMachineType();
    void LoadFacilityRoom();

  //  void FillComboBoxWithData(QComboBox* comboBox, const std::string& locale, const std::string& place);



   private:
    bool IsReady() const;
    void WaitUntilReady();
    bool HasFailed() const;
    std::string GetLastError() const;

    std::unordered_map<std::uint32_t, FacilityRoomData> _facilityRoomById;
    std::unordered_map<CompanyLocations, std::vector<std::uint32_t>> _facilityRoomIdsByLocation;

    std::unordered_map<std::uint32_t, MachineLineData> _machineLineById;
    std::unordered_map<CompanyLocations, std::vector<std::uint32_t>> _machineLineIdsByLocation;

    std::vector<MachineManufacturerData> _machineManufacturerData;
    std::unordered_map<std::uint32_t, std::string> _machineManufacturerNameById;

    std::vector<MachineTypeData> _machineTypeData;
    std::unordered_map<std::uint32_t, std::string> _machineTypeNameById;

    mutable std::shared_mutex _dataMutex;
    template <typename Fn>
    void RunWithRetry(const char* name, Fn&& fn);

    std::atomic_bool _fullyLoaded = false;
    std::atomic_bool _initStarted = false;
    std::atomic_bool _initFailed = false;

    mutable std::mutex _errorMutex;
    std::string _lastError;

    std::future<void> _f1;
    std::future<void> _f2;
    std::future<void> _f3;
    std::future<void> _f4;

    std::jthread _joinThread;
};
