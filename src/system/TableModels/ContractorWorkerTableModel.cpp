#include "pch.h"
#include "ContractorWorkerTableModel.h"
#include "DatabaseDefines.h"

#include <algorithm>
#include <QString>

ContractorWorkerTableModel::ContractorWorkerTableModel(QObject* parent) : QAbstractTableModel(parent) {}

void ContractorWorkerTableModel::setDataRows(std::vector<ContractorWorkerDataDB> rows)
{
    beginResetModel();
    _rows = std::move(rows);
    endResetModel();
}

std::uint32_t ContractorWorkerTableModel::getWorkerId(int row) const
{
    if (row < 0 || row >= static_cast<int>(_rows.size()))
        return 0;

    return _rows[static_cast<size_t>(row)].id;
}

const ContractorWorkerDataDB* ContractorWorkerTableModel::getWorker(int row) const
{
    if (row < 0 || row >= static_cast<int>(_rows.size()))
        return nullptr;

    return &_rows[static_cast<size_t>(row)];
}

int ContractorWorkerTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return static_cast<int>(_rows.size());
}

int ContractorWorkerTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return static_cast<int>(Column::Count);
}

QVariant ContractorWorkerTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return {};

    const int row = index.row();
    const int col = index.column();

    if (row < 0 || row >= rowCount() || col < 0 || col >= columnCount())
        return {};

    const auto& e = _rows[static_cast<size_t>(row)];
    const auto column = static_cast<Column>(col);

    if (role == Qt::DisplayRole)
        return columnText(e, column);

    if (role == Qt::ToolTipRole)
        return columnText(e, column);

    if (role == Qt::TextAlignmentRole)
    {
        if (column == Column::Active)
            return Qt::AlignCenter;

        return static_cast<int>(Qt::AlignVCenter | Qt::AlignLeft);
    }

    return {};
}

QVariant ContractorWorkerTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (static_cast<Column>(section))
    {
        case Column::CompanyName:
            return tr("Company");
        case Column::FirstName:
            return tr("First name");
        case Column::LastName:
            return tr("Last name");
        case Column::Phone:
            return tr("Phone");
        case Column::Email:
            return tr("E-Mail");
        case Column::Note:
            return tr("Note");
        case Column::Active:
            return tr("Active");
        default:
            return {};
    }
}

QString ContractorWorkerTableModel::columnText(const ContractorWorkerDataDB& e, Column col) const
{
    auto s = [](const std::string& v) -> QString { return QString::fromStdString(v); };

    switch (col)
    {
        case Column::CompanyName:
            return s(e.companyName);
        case Column::FirstName:
            return s(e.firstName);
        case Column::LastName:
            return s(e.lastName);
        case Column::Phone:
            return s(e.phone);
        case Column::Email:
            return s(e.email);
        case Column::Note:
            return s(e.note);
        case Column::Active:
            return e.isActive ? tr("Yes") : tr("No");
        default:
            return {};
    }
}

void ContractorWorkerTableModel::sort(int column, Qt::SortOrder order)
{
    if (column < 0 || column >= columnCount())
        return;

    const auto col = static_cast<Column>(column);

    beginResetModel();

    std::ranges::sort(_rows,
                      [&](const ContractorWorkerDataDB& a, const ContractorWorkerDataDB& b)
                      {
                          const QString av = columnText(a, col);
                          const QString bv = columnText(b, col);

                          const int cmp = QString::compare(av, bv, Qt::CaseInsensitive);
                          return (order == Qt::AscendingOrder) ? (cmp < 0) : (cmp > 0);
                      });

    endResetModel();
}
