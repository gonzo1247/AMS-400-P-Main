#include "pch.h"
#include "ContractorVisitWorkerModel.h"

#include <QString>

ContractorVisitWorkerModel::ContractorVisitWorkerModel(QObject* parent) : QAbstractTableModel(parent) {}

void ContractorVisitWorkerModel::setRows(const std::vector<ContractorVisitWorkerInfo>& rows)
{
    beginResetModel();
    _rows = rows;
    endResetModel();
}

void ContractorVisitWorkerModel::clear()
{
    beginResetModel();
    _rows.clear();
    endResetModel();
}

int ContractorVisitWorkerModel::rowCount(const QModelIndex&) const { return static_cast<int>(_rows.size()); }

int ContractorVisitWorkerModel::columnCount(const QModelIndex&) const { return COL_COUNT; }

QVariant ContractorVisitWorkerModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount())
        return {};

    const auto& row = _rows[index.row()];

    if (role == RoleWorkerId)
        return static_cast<qulonglong>(row.workerId);

    if (role != Qt::DisplayRole)
        return {};

    switch (index.column())
    {
        case COL_FIRST_NAME:
            return QString::fromStdString(row.firstName);

        case COL_LAST_NAME:
            return QString::fromStdString(row.lastName);

        case COL_PHONE:
            return QString::fromStdString(row.phone);

        case COL_COMPANY:
            return QString::fromStdString(row.company);

        default:
            return {};
    }
}

QVariant ContractorVisitWorkerModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section)
    {
        case COL_FIRST_NAME:
            return tr("First name");
        case COL_LAST_NAME:
            return tr("Last name");
        case COL_PHONE:
            return tr("Phone");
        case COL_COMPANY:
            return tr("Company");
        default:
            return {};
    }
}

std::uint64_t ContractorVisitWorkerModel::getWorkerId(int row) const
{
    if (row < 0 || static_cast<std::size_t>(row) >= _rows.size())
        return 0;

    return _rows[row].workerId;
}
