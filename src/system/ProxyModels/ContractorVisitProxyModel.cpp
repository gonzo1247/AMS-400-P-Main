#include "pch.h"
#include "ContractorVisitProxyModel.h"
#include "ContractorVisitModel.h"

#include <QAbstractItemModel>

ContractorVisitProxyModel::ContractorVisitProxyModel(QObject* parent) : QSortFilterProxyModel(parent)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setDynamicSortFilter(true);
}

void ContractorVisitProxyModel::setQuickFilterText(const QString& text)
{
    _quick = text.trimmed();
    invalidateFilter();
}

bool ContractorVisitProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    if (_quick.isEmpty())
        return true;

    const auto* m = sourceModel();
    if (!m)
        return true;

    auto contains = [&](int col) -> bool
    {
        const QModelIndex idx = m->index(sourceRow, col, sourceParent);
        const QString v = m->data(idx, Qt::DisplayRole).toString();
        return v.contains(_quick, Qt::CaseInsensitive);
    };

    // Spalten, die für QuickSearch sinnvoll sind:
    return contains(ContractorVisitModel::COL_COMPANY_NAME) || contains(ContractorVisitModel::COL_CONTACT_PERSON) ||
           contains(ContractorVisitModel::COL_WORKER_NAME) || contains(ContractorVisitModel::COL_ACTIVITY) ||
           contains(ContractorVisitModel::COL_LOCATION) || contains(ContractorVisitModel::COL_REACHABLE_PHONE) ||
           contains(ContractorVisitModel::COL_NOTE);
}
