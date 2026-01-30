#include "CallerTableModel.h"

#include <algorithm>

#include "CompanyLocationHandler.h"
#include "CostUnitDataHandler.h"
#include "pch.h"

CallerTableModel::CallerTableModel(QObject* parent) : QAbstractTableModel(parent) {}

void CallerTableModel::setData(const CallerMap& map)
{
    _allRows.clear();
    _allRows.reserve(map.size());

    for (const auto& [id, info] : map)
    {
        // Ensure id field is consistent with key if needed
        CallerInformation copy = info;
        copy.id = id;
        _allRows.push_back(copy);
    }

    rebuildRows();
}

void CallerTableModel::setFilterText(const QString& text)
{
    if (_filterText == text)
        return;

    _filterText = text;
    rebuildRows();
}

void CallerTableModel::setOnlyActive(bool onlyActive)
{
    if (_onlyActive == onlyActive)
        return;

    _onlyActive = onlyActive;
    rebuildRows();
}

int CallerTableModel::rowCount(const QModelIndex&) const { return static_cast<int>(_rows.size()); }

int CallerTableModel::columnCount(const QModelIndex&) const
{
    // id, department, phone, name, costUnitID, companyLocationID, is_active
    return 7;
}

QVariant CallerTableModel::data(const QModelIndex& index, int role) const
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
                return static_cast<qulonglong>(row.id);
            case 1:
                return QString::fromStdString(row.department);
            case 2:
                return QString::fromStdString(row.phone);
            case 3:
                return QString::fromStdString(row.name);
            case 4:
                return QString::fromUtf8(CostUnitDataHandler::instance().GetCostUnitNameByInternalId(row.costUnitID));
            case 5:
                return QString::fromUtf8(CompanyLocationHandler::instance().GetLocationNameById(row.companyLocationID));
            case 6:
                return row.is_active ? "Yes" : "No";
            default:
                return {};
        }
    }

    if (role == Qt::TextAlignmentRole)
    {
        switch (index.column())
        {
            case 0:
            case 2:
                return static_cast<int>(Qt::AlignVCenter | Qt::AlignRight);

            case 6:
                return static_cast<int>(Qt::AlignCenter);

            default:
                return static_cast<int>(Qt::AlignVCenter | Qt::AlignLeft);
        }
    }

    if (role == Qt::CheckStateRole && index.column() == 6)
    {
        return row.is_active ? Qt::Checked : Qt::Unchecked;
    }

    if (role == Qt::UserRole)
    {
        // Always return the primary key here
        return static_cast<qulonglong>(row.id);
    }

    return {};
}

QVariant CallerTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section)
    {
        case 0:
            return tr("ID");
        case 1:
            return tr("Department");
        case 2:
            return tr("Phone");
        case 3:
            return tr("Name");
        case 4:
            return tr("Cost Unit");
        case 5:
            return tr("Location");
        case 6:
            return tr("Active");
        default:
            return {};
    }
}

void CallerTableModel::sort(int column, Qt::SortOrder order)
{
    if (_allRows.empty())
        return;

    auto cmp = [column, order](const CallerInformation& a, const CallerInformation& b) -> bool
    {
        auto compare = [&]() -> int
        {
            switch (column)
            {
                case 0:  // id
                {
                    if (a.id < b.id)
                        return -1;
                    if (a.id > b.id)
                        return 1;
                    return 0;
                }
                case 1:  // department
                {
                    const auto sa = QString::fromStdString(a.department);
                    const auto sb = QString::fromStdString(b.department);
                    int r = QString::localeAwareCompare(sa, sb);
                    if (r < 0)
                        return -1;
                    if (r > 0)
                        return 1;
                    return 0;
                }
                case 2:  // phone
                {
                    const auto sa = QString::fromStdString(a.phone);
                    const auto sb = QString::fromStdString(b.phone);
                    int r = QString::localeAwareCompare(sa, sb);
                    if (r < 0)
                        return -1;
                    if (r > 0)
                        return 1;
                    return 0;
                }
                case 3:  // name
                {
                    const auto sa = QString::fromStdString(a.name);
                    const auto sb = QString::fromStdString(b.name);
                    int r = QString::localeAwareCompare(sa, sb);
                    if (r < 0)
                        return -1;
                    if (r > 0)
                        return 1;
                    return 0;
                }
                case 4:  // costUnitID
                {
                    if (a.costUnitID < b.costUnitID)
                        return -1;
                    if (a.costUnitID > b.costUnitID)
                        return 1;
                    return 0;
                }
                case 5:  // companyLocationID
                {
                    if (a.companyLocationID < b.companyLocationID)
                        return -1;
                    if (a.companyLocationID > b.companyLocationID)
                        return 1;
                    return 0;
                }
                case 6:  // is_active
                {
                    if (a.is_active == b.is_active)
                        return 0;
                    // Active first
                    return a.is_active ? -1 : 1;
                }
                default:
                    return 0;
            }
        };

        int r = compare();
        if (r == 0)
            return false;

        if (order == Qt::AscendingOrder)
            return r < 0;
        else
            return r > 0;
    };

    beginResetModel();
    std::ranges::sort(_allRows, cmp);
    endResetModel();

    // Re-apply filtering on sorted data
    rebuildRows();
}

void CallerTableModel::rebuildRows()
{
    beginResetModel();

    _rows.clear();
    _rows.reserve(_allRows.size());

    const QString filter = _filterText.trimmed();
    const bool hasFilter = !filter.isEmpty();

    for (const auto& info : _allRows)
    {
        if (_onlyActive && !info.is_active)
            continue;

        if (hasFilter)
        {
            const auto dep = QString::fromStdString(info.department);
            const auto phone = QString::fromStdString(info.phone);
            const auto name = QString::fromStdString(info.name);

            const bool match = dep.contains(filter, Qt::CaseInsensitive) ||
                               phone.contains(filter, Qt::CaseInsensitive) ||
                               name.contains(filter, Qt::CaseInsensitive);

            if (!match)
                continue;
        }

        _rows.push_back(info);
    }

    endResetModel();
}
