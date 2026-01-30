#pragma once
#include "ShowTicketManager.h"

#include <QString>

class ShowTicketDetailManager
{
public:
	ShowTicketDetailManager();
    ~ShowTicketDetailManager() = default;

	void LoadTicketDetails(std::uint64_t ticketID);
    ShowTicketData GetTicketData() const { return _ticketData; }

    void AddNewComment(std::uint64_t ticketID, const QString& comment, bool is_internal = true);
    void RemoveComment(std::uint64_t ID);

    void FillTicketStatusBox(QComboBox* cb);
    void FillTicketPriorityBox(QComboBox* cb);

    void UpdatePriority(std::uint64_t ticketID, const TicketPriority& newPriority);
    void UpdateStatus(std::uint64_t ticketID, const TicketStatus& newStatus);

    bool UpdateTicketTitle(std::uint64_t ticketID, const QString& newTitle);
    bool UpdateTicketDescription(std::uint64_t ticketID, const QString& newDescription);

    QString LoadMachineLineName(std::uint32_t lineID);
    QString LoadMachineTypeName(std::uint32_t typeID);
    QString LoadManufacturerName(std::uint32_t manufacturerID);

    void CloseTicket(std::uint64_t ticketID);

private:
	void BuildTimeline(ShowTicketData& data);
    QIcon MakeStatusIcon(const QColor& color);
    QIcon MakePriorityIcon(const QColor& color);

    QColor GetTicketStatusColor(TicketStatus s);
    QString GetTicketStatusTooltip(TicketStatus s);

    QColor GetTicketPriorityColor(TicketPriority p);
    QString GetTicketPriorityTooltip(TicketPriority p);

    QString LoadNameByID(AMSPreparedStatement stmt, std::uint32_t id);

	ShowTicketData _ticketData;
    std::unique_ptr<ShowTicketManager> _ticketManager;


};

