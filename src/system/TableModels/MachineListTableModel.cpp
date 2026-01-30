
#include "pch.h"
#include "MachineListTableModel.h"

#include <algorithm>
#include <ranges>

#include "CompanyLocationHandler.h"
#include "CostUnitDataHandler.h"
#include "MachineDataHandler.h"
#include "SharedDefines.h"


MachineListTableModel::MachineListTableModel(QObject* parent) : QAbstractTableModel(parent) {}

void MachineListTableModel::setData(const std::vector<MachineInformation>& machines)
{
    _allRows = machines;

    _allIndexById.clear();
    _allIndexById.reserve(_allRows.size());

    for (std::size_t i = 0; i < _allRows.size(); ++i)
        _allIndexById[_allRows[i].ID] = i;

    rebuildRows();
}

void MachineListTableModel::setFilterText(const QString& text)
{
    if (_filterText == text)
        return;

    _filterText = text;
    rebuildRows();
}

void MachineListTableModel::setOnlyActive(bool onlyActive)
{
    if (_onlyActive == onlyActive)
        return;

    _onlyActive = onlyActive;
    rebuildRows();
}

int MachineListTableModel::rowCount(const QModelIndex&) const { return static_cast<int>(_rows.size()); }

int MachineListTableModel::columnCount(const QModelIndex&) const
{
    // ID, CostUnitID, MachineTypeID, LineID, ManufacturerID,
    // MachineName, MachineNumber, ManufacturerMachineNumber,
    // RoomID, MoreInformation, locationID, isActive
    return 12;
}

QVariant MachineListTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || static_cast<std::size_t>(index.row()) >= _rows.size())
    {
        return {};
    }

    const auto& row = _rows[static_cast<std::size_t>(index.row())];

    if (role == Qt::DisplayRole)
    {
        switch (index.column())
        {
            case 0:
                return static_cast<qlonglong>(row.ID);
            case 1:
                return QString::fromStdString(CostUnitDataHandler::instance().GetCostUnitNameByInternalId(row.CostUnitID));
            case 2:
                return QString::fromStdString(MachineDataHandler::instance().GetMachineTypeByID(row.MachineTypeID));
            case 3:
                return QString::fromStdString(MachineDataHandler::instance().GetMachineLineNameByID(row.LineID));
            case 4:
                return QString::fromStdString(MachineDataHandler::instance().GetMachineManufacturerNameByID(row.ManufacturerID));
            case 5:
                return QString::fromStdString(row.MachineName);
            case 6:
                return QString::fromStdString(row.MachineNumber);
            case 7:
                return QString::fromStdString(row.ManufacturerMachineNumber);
            case 8:
                return QString::fromStdString(MachineDataHandler::instance().GetFacilityRoomByID(row.RoomID));
            case 9:
                return QString::fromStdString(row.MoreInformation);
            case 10:
                return QString::fromStdString(CompanyLocationHandler::instance().GetLocationNameById(row.locationID));
            case 11:
                return {}; // row.isActive ? QStringLiteral("Yes") : QStringLiteral("No");
            default:
                return {};
        }
    }

    if (role == Qt::CheckStateRole && index.column() == 11)
    {
        return row.isActive ? Qt::Checked : Qt::Unchecked;
    }

    if (role == Qt::UserRole)
    {
        // Primary key for helper functions
        return static_cast<qlonglong>(row.ID);
    }

    if (role == Qt::TextAlignmentRole)
    {
        switch (index.column())
        {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 8:
            case 10:
                return QVariant::fromValue<int>(Qt::AlignVCenter | Qt::AlignRight);

            case 11:
                return QVariant::fromValue<int>(Qt::AlignCenter);

            default:
                return QVariant::fromValue<int>(Qt::AlignVCenter | Qt::AlignLeft);
        }
    }

    return {};
}

QVariant MachineListTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section)
    {
        case 0:
            return tr("ID");
        case 1:
            return tr("Cost Unit");
        case 2:
            return tr("Type");
        case 3:
            return tr("Line");
        case 4:
            return tr("Manufacturer");
        case 5:
            return tr("Name");
        case 6:
            return tr("Machine No.");
        case 7:
            return tr("Manufacturer No.");
        case 8:
            return tr("Room");
        case 9:
            return tr("Information");
        case 10:
            return tr("Location");
        case 11:
            return tr("Active");
        default:
            return {};
    }
}

void MachineListTableModel::sort(int column, Qt::SortOrder order)
{
    _lastSortColumn = column;
    _lastSortOrder = order;

    if (_allRows.empty())
        return;

    if (column < 0 || column >= columnCount({}))
        return;

    auto buildDisplayCache = [&](int col) -> std::unordered_map<std::uint32_t, QString>
    {
        std::unordered_map<std::uint32_t, QString> cache;
        cache.reserve(_allRows.size());

        for (const auto& m : _allRows)
        {
            QString s;

            switch (col)
            {
                case 1:
                    s = QString::fromStdString(
                        CostUnitDataHandler::instance().GetCostUnitNameByInternalId(m.CostUnitID));
                    break;

                case 2:
                    s = QString::fromStdString(MachineDataHandler::instance().GetMachineTypeByID(m.MachineTypeID));
                    break;

                case 3:
                    s = QString::fromStdString(MachineDataHandler::instance().GetMachineLineNameByID(m.LineID));
                    break;

                case 4:
                    s = QString::fromStdString(
                        MachineDataHandler::instance().GetMachineManufacturerNameByID(m.ManufacturerID));
                    break;

                case 5:
                    s = QString::fromStdString(m.MachineName);
                    break;

                case 6:
                    s = QString::fromStdString(m.MachineNumber);
                    break;

                case 7:
                    s = QString::fromStdString(m.ManufacturerMachineNumber);
                    break;

                case 8:
                    s = QString::fromStdString(MachineDataHandler::instance().GetFacilityRoomByID(m.RoomID));
                    break;

                case 9:
                    s = QString::fromStdString(m.MoreInformation);
                    break;

                case 10:
                    s = QString::fromStdString(CompanyLocationHandler::instance().GetLocationNameById(m.locationID));
                    break;

                case 11:
                    s = m.isActive ? tr("Yes") : tr("No");
                    break;

                default:
                    s.clear();
                    break;
            }

            cache[m.ID] = std::move(s);
        }

        return cache;
    };

    const bool sortNumeric = (column == 0);

    std::unordered_map<std::uint32_t, QString> displayCache;
    if (!sortNumeric)
        displayCache = buildDisplayCache(column);

    auto cmp = [&](const MachineInformation& a, const MachineInformation& b) -> bool
    {
        int r = 0;

        if (sortNumeric)
        {
            if (a.ID < b.ID)
                r = -1;
            else if (a.ID > b.ID)
                r = 1;
            else
                r = 0;
        }
        else
        {
            const QString sa = displayCache[a.ID];
            const QString sb = displayCache[b.ID];

            r = QString::localeAwareCompare(sa, sb);

            if (r == 0)
            {
                if (a.ID < b.ID)
                    r = -1;
                else if (a.ID > b.ID)
                    r = 1;
            }
        }

        if (r == 0)
            return false;

        if (order == Qt::AscendingOrder)
            return r < 0;

        return r > 0;
    };

    emit layoutAboutToBeChanged();

    std::ranges::sort(_allRows, cmp);

    _allIndexById.clear();
    _allIndexById.reserve(_allRows.size());
    for (std::size_t i = 0; i < _allRows.size(); ++i)
        _allIndexById[_allRows[i].ID] = i;

    rebuildVisibleRowsNoReset();

    emit layoutChanged();
}

bool MachineListTableModel::IsVisibleByCurrentFilter(const MachineInformation& info) const
{
    if (_onlyActive && !info.isActive)
        return false;

    const QString filter = _filterText.trimmed();
    if (filter.isEmpty())
        return true;

    const auto name = QString::fromStdString(info.MachineName);
    const auto number = QString::fromStdString(info.MachineNumber);
    const auto manuNumber = QString::fromStdString(info.ManufacturerMachineNumber);
    const auto more = QString::fromStdString(info.MoreInformation);

    const auto idStr = QString::number(info.ID);
    const auto roomStr = QString::number(info.RoomID);
    const auto costStr = QString::number(info.CostUnitID);

    return name.contains(filter, Qt::CaseInsensitive) || number.contains(filter, Qt::CaseInsensitive) ||
           manuNumber.contains(filter, Qt::CaseInsensitive) || more.contains(filter, Qt::CaseInsensitive) ||
           idStr.contains(filter, Qt::CaseInsensitive) || roomStr.contains(filter, Qt::CaseInsensitive) ||
           costStr.contains(filter, Qt::CaseInsensitive);
}

bool MachineListTableModel::ApplyMachinePatch(const MachineInformation& updated)
{
    const auto itAll = _allIndexById.find(updated.ID);
    if (itAll == _allIndexById.end())
        return false;

    const bool wasVisible = _visibleIndexById.contains(updated.ID);

    _allRows[itAll->second] = updated;

    const bool nowVisible = IsVisibleByCurrentFilter(updated);
    if (wasVisible != nowVisible)
    {
        rebuildRows();
        return true;
    }

    if (!nowVisible)
        return true;

    const std::size_t rowIndex = _visibleIndexById[updated.ID];
    _rows[rowIndex] = updated;

    if (_lastSortColumn >= 0)
    {
        sort(_lastSortColumn, _lastSortOrder);
        return true;
    }

    const QModelIndex tl = index(static_cast<int>(rowIndex), 0);
    const QModelIndex br = index(static_cast<int>(rowIndex), columnCount({}) - 1);
    emit dataChanged(tl, br, {Qt::DisplayRole, Qt::CheckStateRole, Qt::UserRole});

    return true;
}

void MachineListTableModel::rebuildRows()
{
    beginResetModel();
    rebuildVisibleRowsNoReset();
    endResetModel();
}

void MachineListTableModel::rebuildVisibleRowsNoReset()
{
    _rows.clear();
    _rows.reserve(_allRows.size());

    for (const auto& info : _allRows)
    {
        if (IsVisibleByCurrentFilter(info))
            _rows.push_back(info);
    }

    _visibleIndexById.clear();
    _visibleIndexById.reserve(_rows.size());

    for (std::size_t i = 0; i < _rows.size(); ++i)
        _visibleIndexById[_rows[i].ID] = i;
}
