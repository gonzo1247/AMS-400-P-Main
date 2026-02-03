#pragma once

#include <cstdint>
#include <string>

#include "Duration.h"

enum class CompanyLocations;

// caller_information
struct CallerInformation
{
    std::uint64_t id;
    std::string department;
    std::string phone;
    std::string name;
    std::uint16_t costUnitID;
    std::uint32_t companyLocationID;
    bool is_active;
};

// company_cost_unit
struct CostUnitInformation
{
    std::uint16_t id;
    std::map<std::string /*locale*/, std::string /*name*/> costUnitNames;  // language code -> name
    std::uint32_t costUnitID;
    std::string place;
    std::string barcode;
};

// company_location
struct CompanyLocationDatabase
{
    std::uint32_t id;
    std::string locationAcronym;
    std::string fullName;
};

// employee_information
struct EmployeeInformation
{
    std::uint32_t id{};
    std::string firstName{};
    std::string lastName{};
    std::string phone{};
    std::uint32_t location{};
    bool isActive{};
};

// machine_list
struct MachineInformation
{
    std::int32_t ID{};
    std::uint16_t CostUnitID{};
    std::uint16_t MachineTypeID{};
    std::uint16_t LineID{};
    std::uint16_t ManufacturerID{};
    std::string MachineName{};
    std::string MachineNumber{};
    std::string ManufacturerMachineNumber{};
    std::uint16_t RoomID{};
    std::string MoreInformation{};
    std::uint32_t locationID{};
    bool isActive{}; // is_deleted
    SystemTimePoint deleted_at{};

    bool empty() const
    {
        return ID == 0 && CostUnitID == 0 && MachineTypeID == 0 && LineID == 0 && ManufacturerID == 0 &&
               MachineName.empty() && MachineNumber.empty() && ManufacturerMachineNumber.empty() && RoomID == 0 &&
               MoreInformation.empty() && locationID == 0;
    }

    bool isComplete() const
    {
        return CostUnitID != 0 && MachineTypeID != 0 && LineID != 0 && ManufacturerID != 0 && !MachineName.empty() &&
               !MachineNumber.empty() && !ManufacturerMachineNumber.empty() && RoomID != 0 && locationID != 0;
    }

};

// tickets
struct TicketInformation
{
    std::uint64_t id{};
    std::uint32_t creatorUserID{};
    SystemTimePoint createdAt{};
    std::uint8_t currentStatus{};
    std::uint16_t costUnitID{};
    std::string area{};             // need to move to a index from a own table later
    std::string reporterName{};     // unused
    std::string reporterPhone{};    // unused
    std::uint64_t reporterID{};     // caller_information.id
    std::uint16_t entityID{};       // line or machine id
    std::string title{};
    std::string description{};
    std::uint8_t priority{};
    SystemTimePoint updatedAt{};
    SystemTimePoint closedAt{};
    SystemTimePoint lastStatusChangeAt{};
    bool isDeleted{false};
};

// ticket_assignments
struct TicketAssignmentInformation
{
    std::uint64_t ticketID{};
    std::uint32_t employeeID{};
    SystemTimePoint assignedAt{};
    SystemTimePoint unassignedAt{};
    bool isCurrent{false};
    std::string commentAssigned{};
    std::string commentUnassigned{};
    std::uint32_t assignedByUserID{};
    std::uint32_t unassignedByUserID{};
};

// ticket_attachments
struct TicketAttachmentInformation
{
    std::uint64_t id{};
    std::uint64_t ticketID{};
    std::uint32_t uploaderUserID{};
    SystemTimePoint uploadedAt{};
    std::string originalFilename{};
    std::string storedFileName{};
    std::string filePath{};
    std::string mimeType{};
    std::uint64_t fileSize{};
    std::string description{};
    std::uint64_t encryptedSize{};
    std::string sha256Original{};
    std::string sha256Encrypted{};
    bool isDeleted{false};
};

// ticket_comments
struct TicketCommentInformation
{
    std::uint64_t id{};
    std::uint64_t ticketID{};
    std::uint32_t authorUserID{};
    SystemTimePoint createdAt{};
    SystemTimePoint updatedAt{};
    bool isInternal{false};
    bool isDeleted{false};
    std::string message{};
    std::uint32_t deleterUserID{};
    SystemTimePoint deleteAt{};
};


// ticket_status_history
struct TicketStatusHistoryInformation
{
    std::uint64_t id{};
    std::uint64_t ticketID{};
    std::uint8_t oldStatus{};
    std::uint8_t newStatus{};
    SystemTimePoint changedAt{};
    std::uint32_t changedByUserID{};
    std::string comment{};
};

// contractor_visits
struct ContractorVisitInformation
{
    std::uint64_t id{};
    std::string contractorCompany{};
    std::uint32_t contractorCompanyID{};
    std::string contactPerson {};
    std::string workerName{};
    SystemTimePoint arrival_at {};
    SystemTimePoint departure_at {};
    std::string activity {};
    std::string location {};
    std::string contactPhone{};
    std::string contactNote {};
    std::uint8_t status{};
    std::string note {};
    std::uint32_t creatorUserID {};
    SystemTimePoint createdAt {};
    SystemTimePoint updatedAt {};
};

// ticket_spare_parts_used
struct TicketSparePartUsedInformation
{
    std::uint64_t id{};
    std::uint64_t ticketID{};
    std::uint32_t machineID{};
    std::uint64_t articleID{};
    std::uint16_t quantity{};
    std::string unit{};
    std::string note{};
    std::uint64_t createdByUserID{};
    std::string createdByUserName{};
    SystemTimePoint createdAt{};
    bool isDeleted{false};
    SystemTimePoint deletedAt {};
    std::uint64_t deletedByUserID{};
};

// article_database
struct ArticleDatabase
{
    std::uint32_t ID{};
    std::string manufacturer {};
    std::string articleNumber{};
    std::string articleName {};
    std::string replacementPartNumber {};
    std::string usedInSystems {};
    std::string pathToPictures {};
    std::string articleCosts {};
    std::string suppliedBy {};
    std::string moreInformation {};
    std::string knifeAndDiskData {};
    std::string structure {};
    std::uint16_t quantity {};
    std::uint16_t unit{};
    bool isDeleted {false};
};

//  ticket_report
struct TicketReportData
{
    std::uint64_t id{};
    std::uint64_t ticketID{};
    std::string reportHTML{};
    std::string reportPlain{};
    std::uint32_t createdByUserID{};
    std::string createdByUserName{};
    SystemTimePoint createdAt{};
    std::optional<SystemTimePoint> updatedAt {};
};

// facility_room
struct FacilityRoomData
{
    std::uint32_t id{};
    std::string room_code{};
    std::string room_name{};
    CompanyLocations companyLocation{};
    bool is_deleted{};
    SystemTimePoint deleted_at{};
};

// machine_line
struct MachineLineData
{
    std::uint32_t id{};
    std::string name{};
    CompanyLocations companyLocation{};
    bool is_deleted{};
    SystemTimePoint deleted_at{};
};

// machine_manufacturer
struct MachineManufacturerData
{
    std::uint32_t id{};
    std::string name{};
    bool is_deleted{};
    SystemTimePoint deleted_at{};
};

// machine_type
struct MachineTypeData
{
    std::uint32_t id{};
    std::string type{};
    bool is_deleted{};
    SystemTimePoint deleted_at{};
};

struct ContractorWorkerDataDB
{
    std::uint32_t id{};
    std::string companyName{};
    std::uint32_t companyID{};
    std::string firstName{};
    std::string lastName{};
    std::string phone{};
    std::string email{};
    std::string note{};
    bool isActive{};
    SystemTimePoint created_at {};
    SystemTimePoint updated_at {};
};

struct ContractVisitWorker
{
    std::uint32_t visitID {};
    std::uint32_t workerID{};
};

struct ContractorVisitStatusData
{
    std::uint16_t ID {};
    std::string status_name {};
    std::string description {};
};
