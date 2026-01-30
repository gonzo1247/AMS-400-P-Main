#include "pch.h"

#include <QRegularExpression>
#include <algorithm>
#include <ranges>

#include "AssignEmployeeTableModel.h"

AssignEmployeeTableModel::AssignEmployeeTableModel(QObject* parent) : QAbstractTableModel(parent) {}

void AssignEmployeeTableModel::setRows(const std::vector<EmployeeInformation>& rows)
{
    beginResetModel();
    _allRows = rows;
    endResetModel();

    rebuildRows();
}

void AssignEmployeeTableModel::setFilterText(const QString& text)
{
    if (_filterText == text)
        return;

    _filterText = text;
    rebuildRows();
}

int AssignEmployeeTableModel::rowCount(const QModelIndex&) const { return static_cast<int>(_rows.size()); }

int AssignEmployeeTableModel::columnCount(const QModelIndex&) const
{
    // id, name, isActive
    return 3;
}

QVariant AssignEmployeeTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || static_cast<std::size_t>(index.row()) >= _rows.size())
    {
        return {};
    }

    const auto& row = _rows[static_cast<std::size_t>(index.row())];

    if (role == Qt::ForegroundRole)
    {
        if (row.isActive)
            return QColor(0, 128, 0);  // dark green text
    }

    if (role == Qt::DisplayRole)
    {
        switch (index.column())
        {
            case 0:  // ID
                return static_cast<qulonglong>(row.id);

            case 1:  // Name = lastName + " (" + phone + ")"
            {
                QString name = QString::fromStdString(row.lastName);
                if (!row.phone.empty())
                {
                    name += " (" + QString::fromStdString(row.phone) + ")";
                }
                return name;
            }

            case 2:  // Active
                return row.isActive ? tr("Yes") : tr("No");
        }
    }

    if (role == Qt::UserRole)
    {
        return static_cast<qulonglong>(row.id);
    }

    return {};
}

QVariant AssignEmployeeTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section)
    {
        case 0:
            return tr("ID");
        case 1:
            return tr("Name");
        case 2:
            return tr("Current Assigned");
        default:
            return {};
    }
}

void AssignEmployeeTableModel::sort(int column, Qt::SortOrder order)
{
    if (_allRows.empty())
        return;

    auto cmp = [column, order](const EmployeeInformation& a, const EmployeeInformation& b)
    {
        auto cmpStr = [](const std::string& sa, const std::string& sb)
        { return QString::fromStdString(sa).localeAwareCompare(QString::fromStdString(sb)); };

        int r = 0;

        switch (column)
        {
            case 0:  // ID
                r = (a.id < b.id) ? -1 : (a.id > b.id ? 1 : 0);
                break;

            case 1:  // Name: lastName, then firstName
            {
                r = cmpStr(a.lastName, b.lastName);
                if (r == 0)
                    r = cmpStr(a.firstName, b.firstName);
                break;
            }

            case 2:  // isActive (true before false)
                r = static_cast<int>(a.isActive) - static_cast<int>(b.isActive);
                break;

            default:
                r = 0;
                break;
        }

        return (order == Qt::AscendingOrder) ? (r < 0) : (r > 0);
    };

    beginResetModel();
    std::ranges::sort(_allRows, cmp);
    endResetModel();

    rebuildRows();
}

void AssignEmployeeTableModel::rebuildRows()
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
            const auto tokenLower = token.toLower();

            const QString lastName = QString::fromStdString(row.lastName).toLower();
            const QString firstName = QString::fromStdString(row.firstName).toLower();
            const QString phone = QString::fromStdString(row.phone).toLower();
            const QString idStr = QString::number(row.id);

            bool match = lastName.contains(tokenLower, Qt::CaseInsensitive) ||
                         firstName.contains(tokenLower, Qt::CaseInsensitive) ||
                         phone.contains(tokenLower, Qt::CaseInsensitive) || idStr.contains(tokenLower);

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

std::optional<std::uint32_t> AssignEmployeeTableModel::employeeIdForRow(int row) const
{
    if (row < 0 || row >= static_cast<int>(_rows.size()))
        return std::nullopt;

    return _rows[static_cast<std::size_t>(row)].id;
}

std::optional<std::uint32_t> AssignEmployeeTableModel::employeeIdForIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return std::nullopt;

    const int row = index.row();

    if (row < 0 || row >= static_cast<int>(_rows.size()))
        return std::nullopt;

    return _rows[static_cast<std::size_t>(row)].id;
}

void AssignEmployeeTableModel::setEmployeeActiveByIndex(const QModelIndex& index, bool active)
{
    if (!index.isValid())
        return;

    const int row = index.row();
    if (row < 0 || static_cast<std::size_t>(row) >= _rows.size())
        return;

    auto& viewRow = _rows[static_cast<std::size_t>(row)];
    viewRow.isActive = active;

    auto it = std::ranges::find_if(_allRows, [&](EmployeeInformation& e) { return e.id == viewRow.id; });

    if (it != _allRows.end())
        it->isActive = active;

    const int activeCol = 2;
    emit dataChanged(this->index(row, activeCol), this->index(row, activeCol));

    sort(activeCol, Qt::DescendingOrder);
}

void AssignEmployeeTableModel::setEmployeeActiveById(std::uint32_t employeeId, bool active)
{
    auto itAll = std::ranges::find_if(_allRows, [&](EmployeeInformation& e) { return e.id == employeeId; });

    if (itAll == _allRows.end())
        return;

    itAll->isActive = active;

    for (int row = 0; row < static_cast<int>(_rows.size()); ++row)
    {
        if (_rows[static_cast<std::size_t>(row)].id == employeeId)
        {
            _rows[static_cast<std::size_t>(row)].isActive = active;

            const int activeCol = 2;
            emit dataChanged(index(row, activeCol), index(row, activeCol));
            sort(activeCol, Qt::DescendingOrder);
            break;
        }
    }
}

bool AssignEmployeeTableModel::isEmployeeActiveByIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return false;

    const int row = index.row();
    if (row < 0 || static_cast<std::size_t>(row) >= _rows.size())
        return false;

    return _rows[row].isActive;
}
