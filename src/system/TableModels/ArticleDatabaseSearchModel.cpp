#include "pch.h"

#include <QDateTime>

#include "DatabaseDefines.h"
#include "ArticleDatabaseSearchModel.h"

ArticleDatabaseSearchModel::ArticleDatabaseSearchModel(QObject* parent) : QAbstractTableModel(parent) {}

void ArticleDatabaseSearchModel::SetData(std::vector<ArticleDatabase> rows)
{
    beginResetModel();
    _rows = std::move(rows);
    endResetModel();
}

const ArticleDatabase& ArticleDatabaseSearchModel::GetRow(int row) const
{
    return _rows.at(static_cast<size_t>(row));
}

int ArticleDatabaseSearchModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }

    return static_cast<int>(_rows.size());
}

int ArticleDatabaseSearchModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }

    return static_cast<int>(Column::Count);
}

QVariant ArticleDatabaseSearchModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
    {
        return {};
    }

    const auto& row = _rows.at(static_cast<size_t>(index.row()));

    switch (static_cast<Column>(index.column()))
    {
        case Column::ArticleId:
            return static_cast<qulonglong>(row.ID);

        case Column::ArticleName:
            return QString::fromStdString(row.articleName);

        case Column::ArticleNumber:
            return QString::fromStdString(row.articleNumber);

        case Column::Manufacturer:
            return QString::fromStdString(row.manufacturer);

        case Column::Supplier:
            return QString::fromStdString(row.suppliedBy);

        default:
            return {};
    }
}

QVariant ArticleDatabaseSearchModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
    {
        return {};
    }

    switch (static_cast<Column>(section))
    {
        case Column::ArticleId:
            return tr("Intern ID");
        case Column::ArticleName:
            return tr("Article Name");
        case Column::ArticleNumber:
            return tr("Article Number");
        case Column::Manufacturer:
            return tr("Manufacturer");
        case Column::Supplier:
            return tr("Supplier");
        default:
            return {};
    }
}

Qt::ItemFlags ArticleDatabaseSearchModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return Qt::NoItemFlags;
    }

    Qt::ItemFlags f = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    return f;
}
