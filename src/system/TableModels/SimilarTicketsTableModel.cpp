#include "pch.h"
#include <algorithm>

#include "SimilarTicketsTableModel.h"

SimilarTicketsTableModel::SimilarTicketsTableModel(QObject* parent) : QAbstractTableModel(parent) {}

void SimilarTicketsTableModel::SetResults(std::vector<SimilarReportResult>&& results)
{
    beginResetModel();
    _rows = std::move(results);
    endResetModel();
}

const SimilarReportResult* SimilarTicketsTableModel::GetRow(int row) const
{
    if (row < 0 || row >= static_cast<int>(_rows.size()))
        return nullptr;

    return &_rows[static_cast<std::size_t>(row)];
}

int SimilarTicketsTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return static_cast<int>(_rows.size());
}

int SimilarTicketsTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return ColumnCount;
}

QVariant SimilarTicketsTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return {};

    const int row = index.row();
    const int col = index.column();

    if (row < 0 || row >= static_cast<int>(_rows.size()))
        return {};

    const auto& r = _rows[static_cast<std::size_t>(row)];

    if (role == Qt::DisplayRole)
    {
        switch (col)
        {
            case ColumnTicketId:
                return QVariant::fromValue<qulonglong>(static_cast<qulonglong>(r.ticketId));
            case ColumnTitle:
                return r.title;
            case ColumnUpdatedAt:
                return r.updatedAt;
            case ColumnScore:
                return QString::number(r.score, 'f', 2);
            default:
                return {};
        }
    }

    if (role == Qt::UserRole)
    {
        if (col == ColumnTicketId)
            return QVariant::fromValue<qulonglong>(static_cast<qulonglong>(r.ticketId));

        if (col == ColumnScore)
            return r.score;
    }

    if (role == Qt::TextAlignmentRole)
    {
        if (col == ColumnTicketId || col == ColumnScore)
            return QVariant::fromValue(static_cast<int>(Qt::AlignVCenter | Qt::AlignRight));

        return QVariant::fromValue(static_cast<int>(Qt::AlignVCenter | Qt::AlignLeft));
    }

    return {};
}

QVariant SimilarTicketsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return {};

    if (role != Qt::DisplayRole)
        return {};

    switch (section)
    {
        case ColumnTicketId:
            return tr("Ticket ID");
        case ColumnTitle:
            return tr("Title");
        case ColumnUpdatedAt:
            return tr("Updated");
        case ColumnScore:
            return tr("Score");
        default:
            return {};
    }
}

Qt::ItemFlags SimilarTicketsTableModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void SimilarTicketsTableModel::sort(int column, Qt::SortOrder order)
{
    const auto less = [&](const SimilarReportResult& a, const SimilarReportResult& b)
    {
        switch (column)
        {
            case ColumnTicketId:
                return a.ticketId < b.ticketId;
            case ColumnTitle:
                return a.title.localeAwareCompare(b.title) < 0;
            case ColumnUpdatedAt:
                return a.updatedAt < b.updatedAt;
            case ColumnScore:
                return a.score < b.score;
            default:
                return a.ticketId < b.ticketId;
        }
    };

    beginResetModel();

    std::sort(_rows.begin(), _rows.end(),
              [&](const auto& a, const auto& b)
              {
                  if (order == Qt::AscendingOrder)
                      return less(a, b);

                  return less(b, a);
              });

    endResetModel();
}
