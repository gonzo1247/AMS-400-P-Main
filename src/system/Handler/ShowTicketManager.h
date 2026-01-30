#pragma once

#include "ConnectionGuard.h"
#include "DatabaseDefines.h"
#include "SharedDefines.h"

enum class TicketTimelineType : uint8_t
{
    Assignment,
    Unassignment,
    Attachment,
    Comment,
    CommentRemoved,
    StatusHistory,
    SparePartData,
};

struct TicketTimelineEntry
{
    TicketTimelineType type;
    SystemTimePoint timestamp;

    // Pointer or copied payload
    TicketAssignmentInformation assignment;
    TicketAttachmentInformation attachment;
    TicketCommentInformation comment;
    TicketStatusHistoryInformation statusHistory;
    SparePartsTable sparePartData;

    std::string employeeName{};
};

struct ShowTicketData
{
    TicketInformation ticketInfo{};
    std::vector<TicketAssignmentInformation> ticketAssignment{};
    std::vector<TicketAttachmentInformation> ticketAttachment{};
    std::vector<TicketCommentInformation> ticketComment{};
    std::vector<TicketStatusHistoryInformation> ticketStatusHistory{};
    std::vector<TicketTimelineEntry> timeline{};
    std::vector<SparePartsTable> sparePartsUsed{};

    TicketPriority priority{};
    TicketStatus ticketStatus{};
    CallerInformation callerInfo{};
    std::vector<EmployeeInformation> employeeInfo{};
    MachineInformation machineInfo{};
};

struct TicketRowData
{
    std::uint64_t id{};
    std::string title{};
    std::string area{};
    std::string reporterName{};
    std::string machineName{};
    TicketStatus status{};
    TicketPriority priority{};
    SystemTimePoint createdAt{};
    SystemTimePoint closedAt{};
    SystemTimePoint updatedAt{};
    std::vector<std::string> employeeAssigned{};
    int commentCount{};
    int statusHistoryCount{};
    std::string costUnitName{};
    std::string lastComment {};
};

struct TicketDelta
{
    std::vector<TicketRowData> upserts;     // new or changed
    std::vector<std::uint64_t> removedIds;  // deleted/hidden/closed if you want to remove
    SystemTimePoint newSyncPoint{};
};

class ShowTicketManager
{
public:
	ShowTicketManager();
    ~ShowTicketManager() = default;

    void LoadTableTicketData();
    ShowTicketData LoadTicketDetails(std::uint64_t ticketID);
    const std::unordered_map<std::uint64_t, ShowTicketData>& GetTicketDataMap() const;
    const std::vector<TicketRowData> GetTableTicketVector() const;
    const std::vector<TicketRowData>& GetTableTicketVectorNoCopy() const { return _ticketRowDataList; }
    ShowTicketData GetTicketDataByID(std::uint64_t ticketID);
    ShowTicketData GetFullTicketData() const { return _fullTicketData; }
    TicketDelta LoadTableTicketDelta(const std::string& sinceDb);

private:
    void LoadTicketData(std::uint64_t ticketID, TicketInformation& ticketInfo, const ConnectionGuardAMS& connection);
    void LoadTicketAssignments(std::uint64_t ticketID, std::vector<TicketAssignmentInformation>& ticketAssignment, const ConnectionGuardAMS& connection);
    void LoadTicketAttachments(std::uint64_t ticketID, std::vector<TicketAttachmentInformation>& ticketAttachment, const ConnectionGuardAMS& connection);
    void LoadTicketComments(std::uint64_t ticketID, std::vector<TicketCommentInformation>& ticketComment, const ConnectionGuardAMS& connection);
    void LoadTicketStatusHistory(std::uint64_t ticketID, std::vector<TicketStatusHistoryInformation>& ticketStatusHistory, const ConnectionGuardAMS& connection);
    void LoadCallerInformation(std::uint16_t callerID, CallerInformation& callerInfo, const ConnectionGuardAMS& connection);
    void LoadEmployees(std::uint32_t employeeID, std::vector<EmployeeInformation>& employeeInfo, const ConnectionGuardAMS& connection);
    void LoadMachineData(std::uint32_t machineID, MachineInformation& machineInfo, const ConnectionGuardAMS& connection);
    void LoadTicketSpareData(std::uint64_t ticketID, std::vector<SparePartsTable>& sparePartsUsed,
                             const ConnectionGuardAMS& connection);
    


    std::unordered_map<std::uint64_t, ShowTicketData> ticketDataMap{};
    std::vector<TicketRowData> _ticketRowDataList{};
    ShowTicketData _fullTicketData;
};
