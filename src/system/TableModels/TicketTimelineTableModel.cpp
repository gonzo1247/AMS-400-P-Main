#include "TicketTimelineTableModel.h"

#include <QRegularExpression>
#include <algorithm>
#include <ranges>

#include "SharedDefines.h"
#include "TranslateMessageHelper.h"
#include "UserCache.h"
#include "Utilities/Util.h"
#include "pch.h"

TicketTimelineTableModel::TicketTimelineTableModel(QObject* parent) : QAbstractTableModel(parent) {}

void TicketTimelineTableModel::setEntries(const std::vector<TicketTimelineEntry>& entries)
{
    beginResetModel();
    _allEntries = entries;
    endResetModel();

    rebuildRows();
}

void TicketTimelineTableModel::setFilterText(const QString& text)
{
    if (_filterText == text)
        return;

    _filterText = text;
    rebuildRows();
}

int TicketTimelineTableModel::rowCount(const QModelIndex&) const { return static_cast<int>(_entries.size()); }

int TicketTimelineTableModel::columnCount(const QModelIndex&) const
{
    // timestamp, type, summary, actor
    return 4;
}

QVariant TicketTimelineTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || static_cast<std::size_t>(index.row()) >= _entries.size())
    {
        return {};
    }

    const auto& entry = _entries[static_cast<std::size_t>(index.row())];

    if (role == Qt::ForegroundRole)
    {
        return colorForEntry(entry);
    }

    if (role == Qt::DisplayRole)
    {
        switch (index.column())
        {
            case 0:  // Timestamp
                return Util::FormatDateTimeQString(entry.timestamp);

            case 1:  // Type
                return typeToString(entry.type);

            case 2:  // User
                return actorForEntry(entry);

            case 3:  // Summary / Description
                return summaryForEntry(entry);

            default:
                return {};
        }
    }

    // Prepared for future icon usage - currently not returned
    // if (role == Qt::DecorationRole && index.column() == 1)
    // {
    //     return iconForType(entry.type);
    // }

    if (role == Qt::UserRole)
    {
        // Could expose a simple id here if you add one to TicketTimelineEntry
        return static_cast<int>(entry.type);
    }

    return {};
}

QVariant TicketTimelineTableModel::headerData(int section, Qt::Orientation orient, int role) const
{
    if (orient != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section)
    {
        case 0:
            return TranslateText::translateNew("TicketTimeline", "Time");

        case 1:
            return TranslateText::translateNew("TicketTimeline", "Type");

        case 2:
            return TranslateText::translateNew("TicketTimeline", "User");

        case 3:
            return TranslateText::translateNew("TicketTimeline", "Details");

        default:
            return {};
    }
}

void TicketTimelineTableModel::sort(int column, Qt::SortOrder order)
{
    if (_allEntries.empty())
        return;

    auto cmp = [column, order, this](const TicketTimelineEntry& a, const TicketTimelineEntry& b)
    {
        int r = 0;

        switch (column)
        {
            case 0:  // timestamp
            {
                if (a.timestamp < b.timestamp)
                    r = -1;
                else if (a.timestamp > b.timestamp)
                    r = 1;
                else
                    r = 0;
                break;
            }

            case 1:  // type (by translated name)
            {
                const auto sa = typeToString(a.type);
                const auto sb = typeToString(b.type);
                r = sa.localeAwareCompare(sb);
                break;
            }

            case 2:  // user
            {
                const auto sa = actorForEntry(a);
                const auto sb = actorForEntry(b);
                r = sa.localeAwareCompare(sb);
                break;
            }

            case 3:  // summary text
            {
                const auto sa = summaryForEntry(a);
                const auto sb = summaryForEntry(b);
                r = sa.localeAwareCompare(sb);
                break;
            }

            default:
                r = 0;
                break;
        }

        return order == Qt::AscendingOrder ? (r < 0) : (r > 0);
    };

    beginResetModel();
    std::ranges::sort(_allEntries, cmp);
    endResetModel();

    rebuildRows();
}

void TicketTimelineTableModel::rebuildRows()
{
    beginResetModel();

    _entries.clear();
    _entries.reserve(_allEntries.size());

    const QString filter = _filterText.trimmed();

    if (filter.isEmpty())
    {
        _entries = _allEntries;
        endResetModel();
        return;
    }

    const QStringList tokens = filter.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    for (const auto& entry : _allEntries)
    {
        bool include = true;

        const QString tsString = Util::FormatDateTimeQString(entry.timestamp);
        const QString typeString = typeToString(entry.type);
        const QString summary = summaryForEntry(entry);
        const QString actor = actorForEntry(entry);

        for (const QString& token : tokens)
        {
            bool match = tsString.contains(token, Qt::CaseInsensitive) ||
                         typeString.contains(token, Qt::CaseInsensitive) ||
                         summary.contains(token, Qt::CaseInsensitive) ||
                         actor.contains(token, Qt::CaseInsensitive);

            if (!match)
            {
                include = false;
                break;
            }
        }

        if (include)
            _entries.push_back(entry);
    }

    endResetModel();
}

std::optional<TicketTimelineEntry> TicketTimelineTableModel::entryForRow(int row) const
{
    if (row < 0 || row >= rowCount())
        return std::nullopt;

    return _entries[static_cast<std::size_t>(row)];
}

std::optional<TicketTimelineEntry> TicketTimelineTableModel::entryForIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return std::nullopt;

    const int row = index.row();
    if (row < 0 || row >= rowCount())
        return std::nullopt;

    return _entries[static_cast<std::size_t>(row)];
}

QString TicketTimelineTableModel::typeToString(TicketTimelineType type) const
{
    switch (type)
    {
        case TicketTimelineType::Assignment:
            return TranslateText::translateNew("TicketTimeline", "Assignment");

        case TicketTimelineType::Unassignment:
            return TranslateText::translateNew("TicketTimeline", "Unassignment");

        case TicketTimelineType::Attachment:
            return TranslateText::translateNew("TicketTimeline", "Attachment");

        case TicketTimelineType::Comment:
            return TranslateText::translateNew("TicketTimeline", "Comment");

        case TicketTimelineType::CommentRemoved:
            return TranslateText::translateNew("TicketTimeline", "Comment Deleted");

        case TicketTimelineType::StatusHistory:
            return TranslateText::translateNew("TicketTimeline", "Status change");

        case TicketTimelineType::SparePartData:
            return TranslateText::translateNew("TicketTimeline", "Spare Parts");

        default:
            return TranslateText::translateNew("TicketTimeline", "Unknown");
    }
}

QString TicketTimelineTableModel::summaryForEntry(const TicketTimelineEntry& entry) const
{
    switch (entry.type)
    {
        case TicketTimelineType::Assignment:
        case TicketTimelineType::Unassignment:
        {
            const QString empName = QString::fromStdString(entry.employeeName);

            // Determine whether this timeline entry represents an "unassign" event.
            // An unassign event exists only if:
            //  1) the unassignedAt timestamp is set
            //  2) the timeline entry's timestamp matches the unassignedAt timestamp
            const bool isUnassignEvent = entry.assignment.unassignedAt.time_since_epoch().count() != 0 &&
                                         entry.timestamp == entry.assignment.unassignedAt;

            // -------------------------
            // UNASSIGN EVENT
            // -------------------------
            if (isUnassignEvent)
            {
                QString text = TranslateText::translateNew("TicketTimeline", "%1 unassigned").arg(empName);

                // If there is a specific comment for the unassignment, show it.
                if (!entry.assignment.commentUnassigned.empty())
                {
                    text += " - ";
                    text += QString::fromStdString(entry.assignment.commentUnassigned);
                }

                return text;
            }

            // -------------------------
            // ASSIGN EVENT
            // -------------------------

            QString text = TranslateText::translateNew("TicketTimeline", "%1 assigned").arg(empName);

            // If this timeline entry refers to the assignment time and a comment exists,
            // display the assignment comment.
            if (!entry.assignment.commentAssigned.empty())
            {
                text += " - ";
                text += QString::fromStdString(entry.assignment.commentAssigned);
            }

            // Default fallback text when no assignment comment is provided.
            return text;
        }

        case TicketTimelineType::Attachment:
        {
            if (!entry.attachment.description.empty())
                return QString::fromStdString(entry.attachment.description);

            if (!entry.attachment.originalFilename.empty())
                return QString::fromStdString(entry.attachment.originalFilename);

            return TranslateText::translateNew("TicketTimeline", "File attached");
        }

        case TicketTimelineType::Comment:
        {
            if (!entry.comment.message.empty())
                return QString::fromStdString(entry.comment.message);

            return TranslateText::translateNew("TicketTimeline", "Comment added");
        }

        case TicketTimelineType::CommentRemoved:
        {
            if (!entry.comment.message.empty())
                return QString::fromStdString(entry.comment.message);

            return TranslateText::translateNew("TicketTimeline", "Comment deleted");
        }

        case TicketTimelineType::StatusHistory:
        {
            const QString oldStatus = GetTicketStatusQString(static_cast<TicketStatus>(entry.statusHistory.oldStatus));
            const QString newStatus = GetTicketStatusQString(static_cast<TicketStatus>(entry.statusHistory.newStatus));

            QString text = TranslateText::translateNew("TicketTimeline", "Status changed");
            text += ": ";
            text += oldStatus + " -> " + newStatus;

            if (!entry.statusHistory.comment.empty())
            {
                text += " - ";
                text += QString::fromStdString(entry.statusHistory.comment);
            }

            return text;
        }

        case TicketTimelineType::SparePartData:
        {
            if (!entry.sparePartData.articleName.empty())
                return QString::fromStdString(entry.sparePartData.articleName);

            return TranslateText::translateNew("TicketTimeline", "Spare Part Data");
        }

        default:
            return {};
    }
}

QIcon TicketTimelineTableModel::iconForType(TicketTimelineType type) const
{
    // Placeholder for future icon support
    // Example:
    // switch (type)
    // {
    //     case TicketTimelineType::Assignment:
    //         return QIcon(":/icons/timeline_assignment.svg");
    //     case TicketTimelineType::Attachment:
    //         return QIcon(":/icons/timeline_attachment.svg");
    //     case TicketTimelineType::Comment:
    //         return QIcon(":/icons/timeline_comment.svg");
    //     case TicketTimelineType::StatusHistory:
    //         return QIcon(":/icons/timeline_status.svg");
    //     default:
    //         break;
    // }
    return {};
}

QColor TicketTimelineTableModel::colorForEntry(const TicketTimelineEntry& entry) const
{
    switch (entry.type)
    {
        case TicketTimelineType::Assignment:
        case TicketTimelineType::Unassignment:
        {
            // detect unassigned event
            const bool hasUnassignedTs = entry.assignment.unassignedAt.time_since_epoch().count() != 0;

            const bool isUnassignEvent = hasUnassignedTs && (entry.timestamp == entry.assignment.unassignedAt);

            if (isUnassignEvent)
            {
                // red for unassigned
                return QColor("#CC0000");
            }

            // green for assigned / assignment created
            return QColor("#008800");
        }

        case TicketTimelineType::StatusHistory:
        {
            // optional: blue-ish for status changes
            return QColor("#004C99");
        }

        case TicketTimelineType::Attachment:
        {
            // optional: neutral dark for files
            return QColor("#333333");
        }

        case TicketTimelineType::Comment:
        case TicketTimelineType::CommentRemoved:
        {
            // default text color (let style decide)
            return {};
        }

        default:
            return {};
    }
}

QString TicketTimelineTableModel::actorForEntry(const TicketTimelineEntry& entry) const
{
    std::uint32_t userId = 0;

    switch (entry.type)
    {
        case TicketTimelineType::Attachment:
            userId = entry.attachment.uploaderUserID;
            break;

        case TicketTimelineType::Comment:
            userId = entry.comment.authorUserID;
            break;

        case TicketTimelineType::CommentRemoved:
            userId = entry.comment.deleterUserID;
            break;

        case TicketTimelineType::StatusHistory:
            userId = entry.statusHistory.changedByUserID;
            break;

        case TicketTimelineType::Assignment:
            userId = entry.assignment.assignedByUserID;
            break;

        case TicketTimelineType::Unassignment:
            userId = entry.assignment.unassignedByUserID;
            break;

        case TicketTimelineType::SparePartData:
            userId = entry.sparePartData.spareData.createdByUserID;
            break;

        default:
            return {};
    }

    if (userId == 0)
        return {};

    // Assuming GetUserDataByID returns an object with GetUserLastName()
    // and possibly GetUserFirstName() if you want full name later.
    const auto userData = GetUserDataByID(userId);
    const std::string lastName = userData->GetUserLastName();

    if (lastName.empty())
        return {};

    return QString::fromStdString(lastName);
}

