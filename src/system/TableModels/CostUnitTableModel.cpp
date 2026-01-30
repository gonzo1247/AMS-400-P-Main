
#include "CostUnitTableModel.h"

#include <algorithm>
#include <ranges>

#include "SettingsManager.h"
#include "pch.h"

CostUnitTableModel::CostUnitTableModel(QObject* parent) : QAbstractTableModel(parent) {}

void CostUnitTableModel::setData(const CostUnitMap& map)
{
    _allRows.clear();
    _allRows.reserve(map.size());

    for (const auto& [id, info] : map)
    {
        CostUnitInformation copy = info;
        copy.id = id;
        _allRows.push_back(copy);
    }

    rebuildRows();
}

void CostUnitTableModel::setFilterText(const QString& text)
{
    if (_filterText == text)
        return;

    _filterText = text;
    rebuildRows();
}

int CostUnitTableModel::rowCount(const QModelIndex&) const { return static_cast<int>(_rows.size()); }

int CostUnitTableModel::columnCount(const QModelIndex&) const
{
    // id, costUnitID, name(localized), place, barcode
    return 5;
}

QString CostUnitTableModel::getLocalizedName(const CostUnitInformation& info) const
{
    if (info.costUnitNames.empty())
        return {};

    // Assume GetSettings().getLanguage() returns std::string like "de_DE"
    const std::string lang = GetSettings().getLanguage();

    // Try exact match
    auto it = info.costUnitNames.find(lang);
    if (it != info.costUnitNames.end())
        return QString::fromStdString(it->second);

    // Try generic language part, e.g. "de" from "de_DE"
    const auto pos = lang.find('_');
    if (pos != std::string::npos)
    {
        const std::string generic = lang.substr(0, pos);
        for (const auto& [key, value] : info.costUnitNames)
        {
            // starts with generic language code
            if (key.starts_with(generic))
                return QString::fromStdString(value);
        }
    }

    // Fallback to first available name
    return QString::fromStdString(info.costUnitNames.begin()->second);
}

QVariant CostUnitTableModel::data(const QModelIndex& index, int role) const
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
                return static_cast<qulonglong>(row.costUnitID);
            case 2:
                return getLocalizedName(row);
            case 3:
                return QString::fromStdString(row.place);
            case 4:
                return QString::fromStdString(row.barcode);
            default:
                return {};
        }
    }

    if (role == Qt::UserRole)
    {
        // Always return the primary key here
        return static_cast<qulonglong>(row.id);
    }

    return {};
}

QVariant CostUnitTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section)
    {
        case 0:
            return tr("ID");
        case 1:
            return tr("Cost Unit ID");
        case 2:
            return tr("Name");
        case 3:
            return tr("Place");
        case 4:
            return tr("Barcode");
        default:
            return {};
    }
}

void CostUnitTableModel::sort(int column, Qt::SortOrder order)
{
    if (_allRows.empty())
        return;

    auto cmp = [column, order, this](const CostUnitInformation& a, const CostUnitInformation& b) -> bool
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
                case 1:  // costUnitID
                {
                    if (a.costUnitID < b.costUnitID)
                        return -1;
                    if (a.costUnitID > b.costUnitID)
                        return 1;
                    return 0;
                }
                case 2:  // localized name
                {
                    const auto sa = getLocalizedName(a);
                    const auto sb = getLocalizedName(b);
                    const int r = QString::localeAwareCompare(sa, sb);
                    if (r < 0)
                        return -1;
                    if (r > 0)
                        return 1;
                    return 0;
                }
                case 3:  // place
                {
                    const auto sa = QString::fromStdString(a.place);
                    const auto sb = QString::fromStdString(b.place);
                    const int r = QString::localeAwareCompare(sa, sb);
                    if (r < 0)
                        return -1;
                    if (r > 0)
                        return 1;
                    return 0;
                }
                case 4:  // barcode
                {
                    const auto sa = QString::fromStdString(a.barcode);
                    const auto sb = QString::fromStdString(b.barcode);
                    const int r = QString::localeAwareCompare(sa, sb);
                    if (r < 0)
                        return -1;
                    if (r > 0)
                        return 1;
                    return 0;
                }
                default:
                    return 0;
            }
        };

        const int r = compare();
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

    rebuildRows();
}

void CostUnitTableModel::rebuildRows()
{
    beginResetModel();

    _rows.clear();
    _rows.reserve(_allRows.size());

    const QString filter = _filterText.trimmed();
    const bool hasFilter = !filter.isEmpty();

    for (const auto& info : _allRows)
    {
        if (hasFilter)
        {
            const auto name = getLocalizedName(info);
            const auto place = QString::fromStdString(info.place);
            const auto barcode = QString::fromStdString(info.barcode);
            const auto idStr = QString::number(info.id);
            const auto cuStr = QString::number(info.costUnitID);

            const bool match =
                name.contains(filter, Qt::CaseInsensitive) || place.contains(filter, Qt::CaseInsensitive) ||
                barcode.contains(filter, Qt::CaseInsensitive) || idStr.contains(filter, Qt::CaseInsensitive) ||
                cuStr.contains(filter, Qt::CaseInsensitive);

            if (!match)
                continue;
        }

        _rows.push_back(info);
    }

    endResetModel();
}