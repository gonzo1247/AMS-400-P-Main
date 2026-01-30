#include "pch.h"

#include "ContractorVisitStatusModel.h"
#include "DatabaseDefines.h"

#include <QString>



ContractorVisitStatusModel::ContractorVisitStatusModel(QObject* parent) : QAbstractListModel(parent) {}

int ContractorVisitStatusModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return static_cast<int>(_rows.size());
}

QVariant ContractorVisitStatusModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(_rows.size()))
        return {};

    const auto& row = _rows[index.row()];

    switch (role)
    {
        case Qt::DisplayRole:
            return QString::fromStdString(row.status_name);

        case IdRole:
            return static_cast<quint16>(row.ID);

        case Qt::ToolTipRole:
        case DescriptionRole:
            return QString::fromStdString(row.description);

        default:
            return {};
    }
}

void ContractorVisitStatusModel::setRows(std::vector<ContractorVisitStatusData> rows)
{
    beginResetModel();
    _rows = std::move(rows);
    endResetModel();
}

void ContractorVisitStatusModel::clear()
{
    beginResetModel();
    _rows.clear();
    endResetModel();
}
