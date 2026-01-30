#include "pch.h"
#include "ContractorWorkerProxyModel.h"

#include <QAbstractItemModel>

#include "ContractorWorkerTableModel.h"

ContractorWorkerProxyModel::ContractorWorkerProxyModel(QObject* parent) : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

void ContractorWorkerProxyModel::setQuickFilterText(const QString& text)
{
    _quick = text.trimmed();
    invalidateFilter();
}

void ContractorWorkerProxyModel::setOnlyActive(bool onlyActive)
{
    _onlyActive = onlyActive;
    invalidateFilter();
}

bool ContractorWorkerProxyModel::RowContainsAny(const QString& part, const QStringList& values)
{
    for (const auto& v : values)
    {
        if (v.contains(part, Qt::CaseInsensitive))
            return true;
    }

    return false;
}

bool ContractorWorkerProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    const auto* m = sourceModel();
    if (m == nullptr)
        return true;

    auto col = [&](ContractorWorkerTableModel::Column c) -> QString
    {
        const int columnIndex = static_cast<int>(c);
        return m->index(sourceRow, columnIndex, sourceParent).data(Qt::DisplayRole).toString();
    };

    const QString company = col(ContractorWorkerTableModel::Column::CompanyName);
    const QString first = col(ContractorWorkerTableModel::Column::FirstName);
    const QString last = col(ContractorWorkerTableModel::Column::LastName);
    const QString phone = col(ContractorWorkerTableModel::Column::Phone);
    const QString email = col(ContractorWorkerTableModel::Column::Email);
    const QString note = col(ContractorWorkerTableModel::Column::Note);
    const QString active = col(ContractorWorkerTableModel::Column::Active);

    if (_onlyActive)
    {
        const bool isActive = (active.compare(QStringLiteral("Yes"), Qt::CaseInsensitive) == 0) ||
                              (active == QStringLiteral("1")) ||
                              (active.compare(QStringLiteral("True"), Qt::CaseInsensitive) == 0);

        if (!isActive)
            return false;
    }

    if (_quick.isEmpty())
        return true;

    const QStringList values = {company, first, last, phone, email, note};
    const QStringList parts = _quick.split(' ', Qt::SkipEmptyParts);

    for (const auto& part : parts)
    {
        if (!RowContainsAny(part, values))
            return false;
    }

    return true;
}
