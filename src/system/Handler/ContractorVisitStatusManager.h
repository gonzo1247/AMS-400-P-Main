#pragma once

struct ContractorVisitStatusData;

class ContractorVisitStatusManager final
{
public:
    static const std::vector<ContractorVisitStatusData>& GetAll();
    static const ContractorVisitStatusData* GetById(std::uint16_t id);

    static std::string GetName(std::uint16_t id);
    static std::string GetDescription(std::uint16_t id);

    static void Reload();

   private:
    static void LoadIfNeeded();
    static std::vector<ContractorVisitStatusData> LoadStatus();

   private:
    static std::vector<ContractorVisitStatusData> _cache;
    static std::unordered_map<std::uint16_t, std::size_t> _indexById;
    static bool _loaded;
};
