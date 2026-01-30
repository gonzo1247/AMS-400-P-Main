#include "pch.h"

#include "TicketTableModel.h"

#include <QRegularExpression>
#include <algorithm>
#include <ranges>

#include "SimpleBGDelegate.h"


TicketTableModel::TicketTableModel(QObject* parent) : QAbstractTableModel(parent)
{
}

void TicketTableModel::setRows(const std::vector<TicketRowData>& rows)
{
    beginResetModel();
    _allRows = rows;
    endResetModel();

    rebuildRows();
}

void TicketTableModel::setFilterText(const QString& text)
{
    if (_filterText == text)
        return;

    _filterText = text;
    rebuildRows();
}

int TicketTableModel::rowCount(const QModelIndex&) const { return static_cast<int>(_rows.size()); }

int TicketTableModel::columnCount(const QModelIndex&) const
{
    // id, title, area, reporter, machine, priority, status,
    // created, updated, assigned, commentCount, historyCount, costUnit
    return 14;
}

QVariant TicketTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || static_cast<std::size_t>(index.row()) >= _rows.size())
    {
        return {};
    }

    const auto& row = _rows[index.row()];

    if (role == Qt::DisplayRole)
    {
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
        }
    }

    if (role == Qt::UserRole)
    {
        return static_cast<qulonglong>(row.id);
    }

    constexpr int RoleMinutesOpen = Qt::UserRole + 1000;

    if (role == RoleMinutesOpen)
    {
        if (index.column() != 13)
            return {};

        const bool isClosed =
            (row.status == TicketStatus::TICKET_STATUS_RESOLVED || row.status == TicketStatus::TICKET_STATUS_CLOSED);

        if (isClosed)
            return {};

        const auto minutesOpen = Util::OpenMinutes(row.createdAt, row.closedAt, row.status);
        return static_cast<qlonglong>(minutesOpen);
    }

    return {};
}

QVariant TicketTableModel::headerData(int section, Qt::Orientation orientation, int role) const
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
        default:
            return {};
    }
}

void TicketTableModel::sort(int column, Qt::SortOrder order)
{
    if (_allRows.empty())
        return;

    auto cmp = [column, order](const TicketRowData& a, const TicketRowData& b)
    {
        auto cmpStr = [](const std::string& sa, const std::string& sb)
        { return QString::fromStdString(sa).localeAwareCompare(QString::fromStdString(sb)); };

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

        return order == Qt::AscendingOrder ? r < 0 : r > 0;
    };

    beginResetModel();
    std::ranges::sort(_allRows, cmp);
    endResetModel();

    rebuildRows();
}

void TicketTableModel::rebuildRows()
{
    beginResetModel();

    _rows.clear();
    _rows.reserve(_allRows.size());

    QString filter = _filterText.trimmed();

    if (filter.isEmpty())
    {
        _rows = _allRows;
        endResetModel();
        return;
    }

    QStringList tokens = filter.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

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
                         QString::number(row.id).contains(token);

            // search in assigned employees
            if (!match)
            {
                match =
                    std::ranges::any_of(row.employeeAssigned, [&](const std::string& emp)
                    {
                        return QString::fromStdString(emp).contains(token, Qt::CaseInsensitive);
                    });
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

std::optional<std::uint64_t> TicketTableModel::ticketIdForRow(int row) const
{
    if (row < 0 || row >= _rows.size())
        return std::nullopt;

    return _rows[row].id;
}

std::optional<std::uint64_t> TicketTableModel::ticketIdForIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return std::nullopt;

    const int row = index.row();

    if (row < 0 || row >= _rows.size())
        return std::nullopt;

    return _rows[row].id;
}

void TicketTableModel::refreshOpenDurations()
{
    if (_rows.empty())
        return;

    emit dataChanged(index(0, 13), index(rowCount() - 1, 13),
                     {Qt::DisplayRole, Qt::BackgroundRole, Qt::ForegroundRole, Qt::FontRole});
}
