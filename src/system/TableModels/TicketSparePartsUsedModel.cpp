#include "pch.h"
#include "TicketSparePartsUsedModel.h"

#include <QDateTime>

#include "DatabaseDefines.h"
#include "SharedDefines.h"

TicketSparePartsUsedModel::TicketSparePartsUsedModel(QObject* parent) : QAbstractTableModel(parent) {}

void TicketSparePartsUsedModel::SetData(std::vector<SparePartsTable> rows)
{
    beginResetModel();
    _rows = std::move(rows);
    endResetModel();
}

const SparePartsTable& TicketSparePartsUsedModel::GetRow(int row) const {
    return _rows.at(static_cast<size_t>(row));
}

int TicketSparePartsUsedModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }

    return static_cast<int>(_rows.size());
}

int TicketSparePartsUsedModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }

    return static_cast<int>(Column::Count);
}

QVariant TicketSparePartsUsedModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
    {
        return {};
    }

    const auto& row = _rows.at(static_cast<size_t>(index.row()));

    switch (static_cast<Column>(index.column()))
    {
        case Column::ArticleId:
            return QString::fromStdString(row.articleName);

        case Column::Quantity:
            return row.spareData.quantity;

        case Column::Unit:
            return QString::fromStdString(row.spareData.unit);

        case Column::Note:
            return QString::fromStdString(row.spareData.note);

        case Column::CreatedBy:
            return QString::fromStdString(row.spareData.createdByUserName);

        case Column::CreatedAt:
            return QDateTime::fromSecsSinceEpoch(
                std::chrono::duration_cast<std::chrono::seconds>(row.spareData.createdAt.time_since_epoch()).count());

        case Column::Action:
            return QStringLiteral("Remove");

        default:
            return {};
    }
}

QVariant TicketSparePartsUsedModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
    {
        return {};
    }

    switch (static_cast<Column>(section))
    {
        case Column::ArticleId:
            return tr("Article");
        case Column::Quantity:
            return tr("Qty");
        case Column::Unit:
            return tr("Unit");
        case Column::Note:
            return tr("Note");
        case Column::CreatedBy:
            return tr("User");
        case Column::CreatedAt:
            return tr("Created");
        case Column::Action:
            return tr("Action");
        default:
            return {};
    }
}

Qt::ItemFlags TicketSparePartsUsedModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return Qt::NoItemFlags;
    }

    Qt::ItemFlags f = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    if (static_cast<Column>(index.column()) == Column::Action)
    {
        f |= Qt::ItemIsEditable;
    }

    return f;
}

bool TicketSparePartsUsedModel::RemoveById(std::uint64_t rowId)
{
    for (int i = 0; i < static_cast<int>(_rows.size()); ++i)
    {
        if (_rows[static_cast<size_t>(i)].spareData.id == rowId)
        {
            beginRemoveRows(QModelIndex(), i, i);
            _rows.erase(_rows.begin() + i);
            endRemoveRows();
            return true;
        }
    }

    return false;
}

void TicketSparePartsUsedModel::AppendRow(SparePartsTable row)
{
    const int newRow = static_cast<int>(_rows.size());
    beginInsertRows(QModelIndex(), newRow, newRow);
    _rows.push_back(std::move(row));
    endInsertRows();
}
