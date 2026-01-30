#include "EmployeeTableModel.h"

#include <algorithm>

#include "CompanyLocationHandler.h"
#include "pch.h"

EmployeeTableModel::EmployeeTableModel(QObject* parent) : QAbstractTableModel(parent) {}

void EmployeeTableModel::setData(const EmployeeMap& map)
{
    _allRows.clear();
    _allRows.reserve(map.size());

    for (const auto& [id, info] : map)
    {
        // Ensure id field is consistent with key if needed
        EmployeeInformation copy = info;
        copy.id = id;
        _allRows.push_back(copy);
    }

    rebuildRows();
}

void EmployeeTableModel::setFilterText(const QString& text)
{
    if (_filterText == text)
    {
        return;
    }

    _filterText = text;
    rebuildRows();
}

void EmployeeTableModel::setOnlyActive(bool onlyActive)
{
    if (_onlyActive == onlyActive)
    {
        return;
    }

    _onlyActive = onlyActive;
    rebuildRows();
}

int EmployeeTableModel::rowCount(const QModelIndex&) const { return static_cast<int>(_rows.size()); }

int EmployeeTableModel::columnCount(const QModelIndex&) const
{
    // id, firstName, lastName, phone, location, isActive
    return 6;
}

QVariant EmployeeTableModel::data(const QModelIndex& index, int role) const
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
                return QString::fromUtf8(row.firstName);
            case 2:
                return QString::fromUtf8(row.lastName);
            case 3:
                return QString::fromUtf8(row.phone);
            case 4:
                return QString::fromUtf8(CompanyLocationHandler::instance().GetLocationNameById(row.location));
            case 5:
                return row.isActive ? QStringLiteral("Yes") : QStringLiteral("No");
            default:
                return {};
        }
    }

    if (role == Qt::CheckStateRole && index.column() == 5)
    {
        return row.isActive ? Qt::Checked : Qt::Unchecked;
    }

    if (role == Qt::TextAlignmentRole)
    {
        switch (index.column())
        {
            case 0:  // ID
            case 3:  // Phone
                return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);

            case 5:  // Active
                return static_cast<int>(Qt::AlignCenter);

            default:
                return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
        }
    }

    if (role == Qt::ToolTipRole)
    {
        switch (index.column())
        {
            case 1:
                return QString::fromUtf8(row.firstName);
            case 2:
                return QString::fromUtf8(row.lastName);
            case 4:
                return QString::fromUtf8(CompanyLocationHandler::instance().GetLocationNameById(row.location));
            default:
                return {};
        }
    }

    return {};
}

QVariant EmployeeTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
    {
        return {};
    }

    switch (section)
    {
        case 0:
            return tr("ID");
        case 1:
            return tr("First name");
        case 2:
            return tr("Last name");
        case 3:
            return tr("Phone");
        case 4:
            return tr("Location");
        case 5:
            return tr("Active");
        default:
            return {};
    }
}

void EmployeeTableModel::sort(int column, Qt::SortOrder order)
{
    if (_allRows.empty())
    {
        return;
    }

    auto cmp = [column, order](const EmployeeInformation& a, const EmployeeInformation& b) -> bool
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
                case 1:  // firstName
                {
                    const auto sa = QString::fromUtf8(a.firstName);
                    const auto sb = QString::fromUtf8(b.firstName);
                    const int r = QString::localeAwareCompare(sa, sb);
                    if (r < 0)
                        return -1;
                    if (r > 0)
                        return 1;
                    return 0;
                }
                case 2:  // lastName
                {
                    const auto sa = QString::fromUtf8(a.lastName);
                    const auto sb = QString::fromUtf8(b.lastName);
                    const int r = QString::localeAwareCompare(sa, sb);
                    if (r < 0)
                        return -1;
                    if (r > 0)
                        return 1;
                    return 0;
                }
                case 3:  // phone
                {
                    const auto sa = QString::fromUtf8(a.phone);
                    const auto sb = QString::fromUtf8(b.phone);
                    const int r = QString::localeAwareCompare(sa, sb);
                    if (r < 0)
                        return -1;
                    if (r > 0)
                        return 1;
                    return 0;
                }
                case 4:  // location
                {
                    if (a.location < b.location)
                        return -1;
                    if (a.location > b.location)
                        return 1;
                    return 0;
                }
                case 5:  // isActive
                {
                    if (a.isActive == b.isActive)
                        return 0;
                    // Active first
                    return a.isActive ? -1 : 1;
                }
                default:
                    return 0;
            }
        };

        const int r = compare();
        if (r == 0)
        {
            return false;
        }

        if (order == Qt::AscendingOrder)
        {
            return r < 0;
        }
        else
        {
            return r > 0;
        }
    };

    beginResetModel();
    std::ranges::sort(_allRows, cmp);
    endResetModel();

    // Re-apply filtering on sorted data
    rebuildRows();
}

void EmployeeTableModel::rebuildRows()
{
    beginResetModel();

    _rows.clear();
    _rows.reserve(_allRows.size());

    const QString filter = _filterText.trimmed();
    const bool hasFilter = !filter.isEmpty();

    for (const auto& info : _allRows)
    {
        if (_onlyActive && !info.isActive)
        {
            continue;
        }

        if (hasFilter)
        {
            const auto firstName = QString::fromUtf8(info.firstName);
            const auto lastName = QString::fromUtf8(info.lastName);
            const auto phone = QString::fromUtf8(info.phone);

            const bool match = firstName.contains(filter, Qt::CaseInsensitive) ||
                               lastName.contains(filter, Qt::CaseInsensitive) ||
                               phone.contains(filter, Qt::CaseInsensitive);

            if (!match)
            {
                continue;
            }
        }

        _rows.push_back(info);
    }

    endResetModel();
}
