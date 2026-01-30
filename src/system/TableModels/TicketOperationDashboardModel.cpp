#include "TicketOperationDashboardModel.h"

#include <QRegularExpression>
#include <algorithm>
#include <ranges>
#include <unordered_map>
#include <unordered_set>

#include "pch.h"

namespace
{
    constexpr int RoleTicketId = Qt::UserRole;
    constexpr int RoleMinutesOpen = Qt::UserRole + 1000;
    constexpr int RoleStatusPriority = Qt::UserRole + 1;

    qint64 CalcMinutesOpen(const TicketRowData& row)
    {
        const bool isClosed =
            row.status == TicketStatus::TICKET_STATUS_RESOLVED || row.status == TicketStatus::TICKET_STATUS_CLOSED;

        if (isClosed)
            return 0;

        const auto now = Util::GetCurrentSystemPointTime();
        if (now <= row.createdAt)
            return 0;

        const auto diffSec = std::chrono::duration_cast<std::chrono::seconds>(now - row.createdAt).count();
        return static_cast<qint64>(diffSec / 60);
    }
}  // namespace

TicketOperationDashboardModel::TicketOperationDashboardModel(QObject* parent) : QAbstractTableModel(parent) {}

void TicketOperationDashboardModel::setRows(const std::vector<TicketRowData>& rows)
{
    beginResetModel();
    _allRows = rows;
    endResetModel();

    rebuildRows();
}

void TicketOperationDashboardModel::setFilterText(const QString& text)
{
    const QString trimmed = text.trimmed();
    if (_filterText == trimmed)
        return;

    _filterText = trimmed;
    rebuildRows();
}

int TicketOperationDashboardModel::rowCount(const QModelIndex&) const { return static_cast<int>(_rows.size()); }

int TicketOperationDashboardModel::columnCount(const QModelIndex&) const
{
    // id, title, area, reporter, machine, priority, status,
    // created, updated, assigned, commentCount, historyCount, costUnit, openFor
    return 15;
}

QVariant TicketOperationDashboardModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || static_cast<std::size_t>(index.row()) >= _rows.size())
        return {};

    const auto& row = _rows[index.row()];

    if (role == RoleTicketId)
        return static_cast<qulonglong>(row.id);

    if (role == RoleMinutesOpen)
    {
        if (index.column() != 13)
            return {};
        return CalcMinutesOpen(row);
    }

    if (role == RoleStatusPriority)
    {
        if (index.column() != 5 && index.column() != 6)
            return {};

        if (index.column() == 6)
            return static_cast<int>(row.status);

        if (index.column() == 5)
            return static_cast<int>(row.priority);
    }

    if (role == Qt::TextAlignmentRole)
    {
        switch (index.column())
        {
            case 0:
            case 10:
            case 11:
                return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);

            case 7:
            case 8:
            case 13:
                return Qt::AlignCenter;
            case 14:
                return static_cast<int>(Qt::AlignLeft | Qt::AlignTop);

            default:
                return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
        }
    }


    if (role != Qt::DisplayRole)
        return {};

    switch (index.column())
    {
        case 0:
            return static_cast<qulonglong>(row.id);

        case 1:
            return QString::fromStdString(row.title);

        case 2:
            return QString::fromStdString(row.area);

        case 3:
            return QString::fromStdString(row.reporterName);

        case 4:
            return QString::fromStdString(row.machineName);

        case 5:
            return GetTicketPriorityQString(row.priority);

        case 6:
            return GetTicketStatusQString(row.status);

        case 7:
            return Util::FormatDateTimeQString(row.createdAt);

        case 8:
            return Util::FormatDateTimeQString(row.updatedAt);

        case 9:
        {
            QString s;
            for (const auto& emp : row.employeeAssigned)
            {
                if (!s.isEmpty())
                    s += ", ";
                s += QString::fromStdString(emp);
            }
            return s;
        }

        case 10:
            return row.commentCount;

        case 11:
            return row.statusHistoryCount;

        case 12:
            return QString::fromStdString(row.costUnitName);

        case 13:
            return Util::FormatOpenDurationQString(row.createdAt, row.closedAt, row.status);

        case 14:
            return QString::fromStdString(row.lastComment);

        default:
            return {};
    }
}

QVariant TicketOperationDashboardModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section)
    {
        case 0:
            return tr("ID");
        case 1:
            return tr("Title");
        case 2:
            return tr("Area");
        case 3:
            return tr("Reporter");
        case 4:
            return tr("Machine");
        case 5:
            return tr("Priority");
        case 6:
            return tr("Status");
        case 7:
            return tr("Created");
        case 8:
            return tr("Updated");
        case 9:
            return tr("Assigned");
        case 10:
            return tr("Comments");
        case 11:
            return tr("History");
        case 12:
            return tr("Cost Unit");
        case 13:
            return tr("Open For");
        case 14:
            return tr("Comment");
        default:
            return {};
    }
}

void TicketOperationDashboardModel::refreshOpenDurations()
{
    if (_rows.empty())
        return;

    const QModelIndex tl = this->index(0, 13);
    const QModelIndex br = this->index(rowCount() - 1, 13);

    emit dataChanged(tl, br, {Qt::DisplayRole, RoleMinutesOpen});
}

void TicketOperationDashboardModel::ApplyDelta(const std::vector<TicketRowData>& upserts, const std::vector<std::uint64_t>& removedIds)
{
    if (upserts.empty() && removedIds.empty())
        return;

    // Remove
    if (!removedIds.empty() && !_allRows.empty())
    {
        std::unordered_set<std::uint64_t> removeSet;
        removeSet.reserve(removedIds.size());
        for (auto id : removedIds)
            removeSet.insert(id);

        _allRows.erase(
            std::ranges::remove_if(_allRows, [&](const TicketRowData& r) { return removeSet.contains(r.id); }).begin(),
            _allRows.end());
    }

    // Upsert
    if (!upserts.empty())
    {
        std::unordered_map<std::uint64_t, std::size_t> indexById;
        indexById.reserve(_allRows.size());

        for (std::size_t i = 0; i < _allRows.size(); ++i)
            indexById.emplace(_allRows[i].id, i);

        for (const auto& row : upserts)
        {
            auto it = indexById.find(row.id);
            if (it != indexById.end())
            {
                _allRows[it->second] = row;
            }
            else
            {
                indexById.emplace(row.id, _allRows.size());
                _allRows.push_back(row);
            }
        }
    }

    rebuildRows();
}

void TicketOperationDashboardModel::rebuildRows()
{
    beginResetModel();

    _rows.clear();
    _rows.reserve(_allRows.size());

    const QString filter = _filterText.trimmed();
    if (filter.isEmpty())
    {
        _rows = _allRows;
        endResetModel();
        return;
    }

    const QStringList tokens = filter.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    for (const auto& row : _allRows)
    {
        bool include = true;

        for (const auto& token : tokens)
        {
            bool match = QString::fromStdString(row.title).contains(token, Qt::CaseInsensitive) ||
                         QString::fromStdString(row.area).contains(token, Qt::CaseInsensitive) ||
                         QString::fromStdString(row.reporterName).contains(token, Qt::CaseInsensitive) ||
                         QString::fromStdString(row.machineName).contains(token, Qt::CaseInsensitive) ||
                         QString::fromStdString(row.costUnitName).contains(token, Qt::CaseInsensitive) ||
                         QString::number(static_cast<qulonglong>(row.id)).contains(token);

            if (!match)
            {
                match =
                    std::ranges::any_of(row.employeeAssigned, [&](const std::string& emp)
                                        { return QString::fromStdString(emp).contains(token, Qt::CaseInsensitive); });
            }

            if (!match)
            {
                include = false;
                break;
            }
        }

        if (include)
            _rows.push_back(row);
    }

    endResetModel();
}

void TicketOperationDashboardModel::sort(int column, Qt::SortOrder order)
{
    if (_allRows.empty())
        return;

    auto cmpStr = [](const std::string& a, const std::string& b)
    { return QString::fromStdString(a).localeAwareCompare(QString::fromStdString(b)); };

    auto cmp = [=](const TicketRowData& a, const TicketRowData& b)
    {
        int r = 0;

        switch (column)
        {
            case 0:
                r = (a.id < b.id) ? -1 : (a.id > b.id ? 1 : 0);
                break;
            case 1:
                r = cmpStr(a.title, b.title);
                break;
            case 2:
                r = cmpStr(a.area, b.area);
                break;
            case 3:
                r = cmpStr(a.reporterName, b.reporterName);
                break;
            case 4:
                r = cmpStr(a.machineName, b.machineName);
                break;
            case 5:
                r = (int)a.priority - (int)b.priority;
                break;
            case 6:
                r = (int)a.status - (int)b.status;
                break;
            case 7:
                r = (a.createdAt < b.createdAt) ? -1 : (a.createdAt > b.createdAt ? 1 : 0);
                break;
            case 8:
                r = (a.updatedAt < b.updatedAt) ? -1 : (a.updatedAt > b.updatedAt ? 1 : 0);
                break;
            case 10:
                r = a.commentCount - b.commentCount;
                break;
            case 11:
                r = a.statusHistoryCount - b.statusHistoryCount;
                break;
            default:
                r = 0;
                break;
        }

        return (order == Qt::AscendingOrder) ? (r < 0) : (r > 0);
    };

    beginResetModel();
    std::ranges::sort(_allRows, cmp);
    rebuildRows();
    endResetModel();
}
