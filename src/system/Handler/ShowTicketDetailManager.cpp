#include "pch.h"
#include "ShowTicketDetailManager.h"

#include "UserManagement.h"

ShowTicketDetailManager::ShowTicketDetailManager() : _ticketManager(nullptr)
{}

void ShowTicketDetailManager::LoadTicketDetails(std::uint64_t ticketID)
{
    if (!_ticketManager)
        _ticketManager = std::make_unique<ShowTicketManager>();

    _ticketData = _ticketManager->LoadTicketDetails(ticketID);
    BuildTimeline(_ticketData);
}

void ShowTicketDetailManager::AddNewComment(std::uint64_t ticketID, const QString& comment, bool is_internal /*= true*/)
{
    // INSERT INTO ticket_comments (ticket_id, author_user_id, created_at, is_internal, message) VALUES (?, ?, ?, ?, ?)
    ConnectionGuardAMS connection(ConnectionType::Sync);

    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_TC_INSERT_NEW_TICKET_COMMENT);
    stmt->SetUInt64(0, ticketID);
    stmt->SetUInt(1, GetUser().GetUserID());
    stmt->SetCurrentDate(2);
    stmt->SetBool(3, is_internal);
    stmt->SetQString(4, comment);

    connection->ExecutePreparedInsert(*stmt);
}

void ShowTicketDetailManager::RemoveComment(std::uint64_t ID)
{
    // UPDATE ticket_comments SET is_deleted = 1, delete_user_id = ?, delete_at = ? WHERE ID = ?

    ConnectionGuardAMS connection(ConnectionType::Sync);

    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_TC_UPDATE_COMMENT_MARK_AS_DELETED);
    stmt->SetUInt(0, GetUser().GetUserID());
    stmt->SetCurrentDate(1);
    stmt->SetUInt64(2, ID);

    connection->ExecutePreparedUpdate(*stmt);
}

void ShowTicketDetailManager::FillTicketStatusBox(QComboBox* cb)
{
    cb->clear();

    const std::array<TicketStatus, 8> allStatus =
    {
        TicketStatus::TICKET_STATUS_NONE,
        TicketStatus::TICKET_STATUS_NEW,
        TicketStatus::TICKET_STATUS_IN_PROGRESS,
        TicketStatus::TICKET_STATUS_SERVICE_SCHEDULED,
        TicketStatus::TICKET_STATUS_WAITING_FOR_PARTS,
        TicketStatus::TICKET_STATUS_WAITING_FOR_PRODUCTION_RESPONSE,
        TicketStatus::TICKET_STATUS_RESOLVED,
        TicketStatus::TICKET_STATUS_CLOSED
    };

for (TicketStatus s : allStatus)
    {
        const QString text = GetTicketStatusQString(s);
        const QColor color = GetTicketStatusColor(s);
        const QIcon icon = MakeStatusIcon(color);
        const QString tooltip = GetTicketStatusTooltip(s);

        const int enumValue = static_cast<int>(s);

        cb->addItem(icon, text, enumValue);
        const int index = cb->count() - 1;

        cb->setItemData(index, tooltip, Qt::ToolTipRole);
    }
}

void ShowTicketDetailManager::FillTicketPriorityBox(QComboBox* cb)
{
    cb->clear();

    const std::array<TicketPriority, 6> allPriority =
    {
        TicketPriority::TICKET_PRIORITY_NONE,
        TicketPriority::TICKET_PRIORITY_CRITICAL,
        TicketPriority::TICKET_PRIORITY_HIGH,
        TicketPriority::TICKET_PRIORITY_MEDIUM,
        TicketPriority::TICKET_PRIORITY_LOW,
        TicketPriority::TICKET_PRIORITY_TRIVIAL
    };

    for (TicketPriority p : allPriority)
    {
        const QString label = GetTicketPriorityQString(p);
        const QColor color = GetTicketPriorityColor(p);
        const QIcon icon = MakePriorityIcon(color);
        const QString tooltip = GetTicketPriorityTooltip(p);

        const int enumValue = static_cast<int>(p);

        cb->addItem(icon, label, enumValue);
        const int idx = cb->count() - 1;

        cb->setItemData(idx, tooltip, Qt::ToolTipRole);
    }
}

void ShowTicketDetailManager::UpdatePriority(std::uint64_t ticketID, const TicketPriority& newPriority)
{
    ConnectionGuardAMS connection(ConnectionType::Async);

    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_TICKET_UPDATE_TICKET_PRIORITY_BY_ID);
    stmt->SetUInt(0, static_cast<std::uint32_t>(newPriority));
    stmt->SetUInt64(1, ticketID);

    connection->ExecutePreparedUpdate(*stmt);
}

void ShowTicketDetailManager::UpdateStatus(std::uint64_t ticketID, const TicketStatus& newStatus)
{
    ConnectionGuardAMS connection(ConnectionType::Async);

    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_TICKET_UPDATE_TICKET_STATUS_BY_ID);
    stmt->SetUInt(0, static_cast<std::uint32_t>(newStatus));
    stmt->SetUInt64(1, ticketID);

    connection->ExecutePreparedUpdate(*stmt);
}

bool ShowTicketDetailManager::UpdateTicketTitle(std::uint64_t ticketID, const QString& newTitle)
{
    ConnectionGuardAMS connection(ConnectionType::Async);

    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_TICKET_UPDATE_TICKET_TITLE_BY_ID);
    stmt->SetQString(0, newTitle);
    stmt->SetUInt64(1, ticketID);

    return connection->ExecutePreparedUpdate(*stmt);
}

bool ShowTicketDetailManager::UpdateTicketDescription(std::uint64_t ticketID, const QString& newDescription)
{
    ConnectionGuardAMS connection(ConnectionType::Async);

    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_TICKET_UPDATE_TICKET_DESCRIPTION_BY_ID);
    stmt->SetQString(0, newDescription);
    stmt->SetUInt64(1, ticketID);

    return connection->ExecutePreparedUpdate(*stmt);
}

QString ShowTicketDetailManager::LoadMachineLineName(std::uint32_t lineID)
{
    // SELECT Line FROM machine_line WHERE ID = ?

    return LoadNameByID(AMSPreparedStatement::DB_ML_SELECT_NAME_BY_ID, lineID);
}

QString ShowTicketDetailManager::LoadMachineTypeName(std::uint32_t typeID)
{
    // DB_MT_SELECT_NAME_BY_ID, "SELECT Type FROM machine_type WHERE ID = ?

    return LoadNameByID(AMSPreparedStatement::DB_MT_SELECT_NAME_BY_ID, typeID);
}

QString ShowTicketDetailManager::LoadManufacturerName(std::uint32_t manufacturerID)
{
    // DB_MM_SELECT_NAME_BY_ID, "SELECT Name FROM machine_manufacturer WHERE ID = ?
    return LoadNameByID(AMSPreparedStatement::DB_MM_SELECT_NAME_BY_ID, manufacturerID);
}

void ShowTicketDetailManager::CloseTicket(std::uint64_t ticketID)
{
    // simply we set current_status and closed_at
    // UPDATE tickets Set current_status = ?, closed_at = ? WHERE ID = ?
    ConnectionGuardAMS connection(ConnectionType::Async);
    auto stmt = connection->GetPreparedStatement(AMSPreparedStatement::DB_TICKET_UPDATE_TICKET_TO_CLOSED_BY_ID);
    stmt->SetUInt(0, static_cast<std::uint32_t>(TicketStatus::TICKET_STATUS_CLOSED));
    stmt->SetCurrentDate(1);
    stmt->SetUInt64(2, ticketID);

    void(connection->ExecutePreparedUpdate(*stmt));
}


void ShowTicketDetailManager::BuildTimeline(ShowTicketData& data)
{
    data.timeline.clear();

    // --- Assignments ---
    for (const auto& a : data.ticketAssignment)
    {
        TicketTimelineEntry t;
        t.type = TicketTimelineType::Assignment;
        t.timestamp = a.assignedAt;
        t.assignment = a;

        // Find employee name
        auto it = std::ranges::find_if(data.employeeInfo, [&](const EmployeeInformation& emp)
        {
            return emp.id == a.employeeID;
        });

        std::string employeeName {};

        if (it != data.employeeInfo.end())
            employeeName = it->lastName + " (" + it->phone + ")";

        t.employeeName = employeeName;


        data.timeline.push_back(std::move(t));

        if (a.unassignedAt.time_since_epoch().count() != 0)
        {
            TicketTimelineEntry u;
            u.type = TicketTimelineType::Unassignment;
            u.timestamp = a.unassignedAt;
            u.assignment = a;
            u.employeeName = employeeName;

            data.timeline.push_back(std::move(u));
        }
    }

    // --- Attachments ---
    for (const auto& a : data.ticketAttachment)
    {
        TicketTimelineEntry t;
        t.type = TicketTimelineType::Attachment;
        t.timestamp = a.uploadedAt;
        t.attachment = a;
        data.timeline.push_back(std::move(t));
    }

    // --- Comments ---
    for (const auto& c : data.ticketComment)
    {
        TicketTimelineEntry t;
        t.type = TicketTimelineType::Comment;
        t.timestamp = c.createdAt;
        t.comment = c;

        data.timeline.push_back(std::move(t));

        if (c.deleteAt.time_since_epoch().count() != 0)
        {
            TicketTimelineEntry u;
            u.type = TicketTimelineType::CommentRemoved;
            u.timestamp = c.deleteAt;
            u.comment = c;

            data.timeline.push_back(std::move(u));
        }
    }

    // --- Status History ---
    for (const auto& s : data.ticketStatusHistory)
    {
        TicketTimelineEntry t;
        t.type = TicketTimelineType::StatusHistory;
        t.timestamp = s.changedAt;
        t.statusHistory = s;
        data.timeline.push_back(std::move(t));
    }

    for (const auto& sp : data.sparePartsUsed)
    {
        TicketTimelineEntry t;
        t.type = TicketTimelineType::SparePartData;
        t.timestamp = sp.spareData.createdAt;
        t.sparePartData = sp;
        data.timeline.push_back(std::move(t));
    }


    // sort newest to oldest 
    std::ranges::stable_sort(data.timeline, [](const auto& a, const auto& b)
    {
        return a.timestamp > b.timestamp;
    });
}

QIcon ShowTicketDetailManager::MakeStatusIcon(const QColor& color)
{
    constexpr int size = 14;
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);

    p.setPen(Qt::NoPen);
    p.setBrush(color);

    const int margin = 2;
    QRect r(margin, margin, size - 2 * margin, size - 2 * margin);
    p.drawEllipse(r);

    p.end();
    return QIcon(pix);
}

QIcon ShowTicketDetailManager::MakePriorityIcon(const QColor& color)
{
    constexpr int size = 14;
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);

    p.setPen(Qt::NoPen);
    p.setBrush(color);

    const int margin = 2;
    QRect r(margin, margin, size - 2 * margin, size - 2 * margin);
    p.drawEllipse(r);

    p.end();
    return QIcon(pix);
}

QColor ShowTicketDetailManager::GetTicketStatusColor(TicketStatus s)
{
    switch (s)
    {
        case TicketStatus::TICKET_STATUS_NEW:
            return QColor(52, 152, 219);  // blue
        case TicketStatus::TICKET_STATUS_IN_PROGRESS:
            return QColor(241, 196, 15);  // yellow
        case TicketStatus::TICKET_STATUS_SERVICE_SCHEDULED:
            return QColor(155, 89, 182);  // purple
        case TicketStatus::TICKET_STATUS_WAITING_FOR_PARTS:
            return QColor(230, 126, 34);  // orange
        case TicketStatus::TICKET_STATUS_WAITING_FOR_PRODUCTION_RESPONSE:
            return QColor(211, 84, 0);  // dark orange
        case TicketStatus::TICKET_STATUS_RESOLVED:
            return QColor(46, 204, 113);  // green
        case TicketStatus::TICKET_STATUS_CLOSED:
            return QColor(127, 140, 141);  // gray
        case TicketStatus::TICKET_STATUS_NONE:
        default:
            return QColor(149, 165, 166);  // light gray
    }
}

QString ShowTicketDetailManager::GetTicketStatusTooltip(TicketStatus s)
{
    switch (s)
    {
        case TicketStatus::TICKET_STATUS_NEW:
            return QObject::tr("Ticket has been created but not yet processed.");

        case TicketStatus::TICKET_STATUS_IN_PROGRESS:
            return QObject::tr("Ticket is currently being worked on.");

        case TicketStatus::TICKET_STATUS_SERVICE_SCHEDULED:
            return QObject::tr("A service appointment has been scheduled.");

        case TicketStatus::TICKET_STATUS_WAITING_FOR_PARTS:
            return QObject::tr("Waiting for required spare parts.");

        case TicketStatus::TICKET_STATUS_WAITING_FOR_PRODUCTION_RESPONSE:
            return QObject::tr("Waiting for response from production.");

        case TicketStatus::TICKET_STATUS_RESOLVED:
            return QObject::tr("Issue has been resolved, waiting for confirmation.");

        case TicketStatus::TICKET_STATUS_CLOSED:
            return QObject::tr("Ticket is closed.");

        case TicketStatus::TICKET_STATUS_NONE:
        default:
            return QObject::tr("No status assigned.");
    }
}

QColor ShowTicketDetailManager::GetTicketPriorityColor(TicketPriority p)
{
    switch (p)
    {
        case TicketPriority::TICKET_PRIORITY_CRITICAL:
            return QColor(192, 57, 43);  // red
        case TicketPriority::TICKET_PRIORITY_HIGH:
            return QColor(230, 126, 34);  // orange
        case TicketPriority::TICKET_PRIORITY_MEDIUM:
            return QColor(241, 196, 15);  // yellow
        case TicketPriority::TICKET_PRIORITY_LOW:
            return QColor(46, 204, 113);  // green
        case TicketPriority::TICKET_PRIORITY_TRIVIAL:
            return QColor(127, 140, 141);  // gray
        case TicketPriority::TICKET_PRIORITY_NONE:
        default:
            return QColor(149, 165, 166);  // light gray
    }
}

QString ShowTicketDetailManager::GetTicketPriorityTooltip(TicketPriority p)
{
    switch (p)
    {
        case TicketPriority::TICKET_PRIORITY_CRITICAL:
            return QObject::tr("Must be addressed immediately.");

        case TicketPriority::TICKET_PRIORITY_HIGH:
            return QObject::tr("High importance; should be processed soon.");

        case TicketPriority::TICKET_PRIORITY_MEDIUM:
            return QObject::tr("Normal priority.");

        case TicketPriority::TICKET_PRIORITY_LOW:
            return QObject::tr("Can be handled when time allows.");

        case TicketPriority::TICKET_PRIORITY_TRIVIAL:
            return QObject::tr("Very low importance; optional.");

        case TicketPriority::TICKET_PRIORITY_NONE:
        default:
            return QObject::tr("No priority assigned.");
    }
}

QString ShowTicketDetailManager::LoadNameByID(AMSPreparedStatement aps, std::uint32_t id)
{
    ConnectionGuardAMS connection(ConnectionType::Sync);
    auto stmt = connection->GetPreparedStatement(aps);
    stmt->SetUInt(0, id);

    auto result = connection->ExecutePreparedSelect(*stmt);

    if (!result.IsValid())
        return QString();

    if (result.Next())
    {
        Field* fields = result.Fetch();
        return fields[0].GetQString();
    }

    return QString();
}
