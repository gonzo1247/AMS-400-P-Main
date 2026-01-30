#pragma once

struct SparePartsTable;
struct TicketSparePartUsedInformation;

class SparePartUsedManager
{
public:
    explicit SparePartUsedManager();
    ~SparePartUsedManager() = default;

    void LoadAllSparePartsData();
    std::vector<SparePartsTable> LoadSparePartsDataForTicket(std::uint64_t ticketID);
    bool SoftDeleteSparePartUsedById(std::uint64_t rowId, std::uint64_t deletedBy);
    std::optional<std::uint64_t> InsertSparePartUsed(std::uint64_t ticketId, std::uint32_t machineId,
                                                     std::uint64_t articleId, std::uint16_t quantity,
                                                     const std::string& unit, const std::string& note,
                                                     std::uint64_t createdBy, const std::string& createdByName);

private:
    std::vector<SparePartsTable> LoadArticleName(const std::vector<TicketSparePartUsedInformation>& sparePartsUsedList);


    std::vector<TicketSparePartUsedInformation> _sparePartsUsedList;
};
