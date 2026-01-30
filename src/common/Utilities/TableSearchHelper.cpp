#include "pch.h"
#include "TableSearchHelper.h"

#include <QRegularExpression>

QSortFilterProxyModel* TableSearchHelper::Setup(QTableView* view, QLineEdit* searchEdit,
                                                QAbstractItemModel* sourceModel, int filterColumn /* = -1*/)
{
    if (!view || !searchEdit || !sourceModel)
        return nullptr;

    auto* proxy = new QSortFilterProxyModel(view);
    proxy->setSourceModel(sourceModel);
    proxy->setFilterKeyColumn(filterColumn);
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxy->setDynamicSortFilter(true);

    view->setModel(proxy);
    view->setSortingEnabled(true);

    QObject::connect(searchEdit, &QLineEdit::textChanged, view,
                     [proxy](const QString& text)
                     {
                         const QString needle = text.trimmed();

                         if (needle.isEmpty())
                         {
                             proxy->setFilterRegularExpression(QRegularExpression());
                             return;
                         }

                         const QRegularExpression re(QRegularExpression::escape(needle),
                                                     QRegularExpression::CaseInsensitiveOption);

                         proxy->setFilterRegularExpression(re);
                     });

    return proxy;
}

QModelIndex TableSearchHelper::MapToSource(const QTableView* view, const QModelIndex& viewIndex)
{
    if (!viewIndex.isValid())
        return {};

    auto* proxy = GetProxy(view);
    if (!proxy)
        return viewIndex;

    return proxy->mapToSource(viewIndex);
}

int TableSearchHelper::MapRowToSource(const QTableView* view, int viewRow)
{
    if (!view)
        return -1;

    const QModelIndex viewIdx = view->model()->index(viewRow, 0);
    const QModelIndex srcIdx = MapToSource(view, viewIdx);
    return srcIdx.isValid() ? srcIdx.row() : -1;
}

QSortFilterProxyModel* TableSearchHelper::GetProxy(const QTableView* view)
{
    if (!view)
        return nullptr;

    return qobject_cast<QSortFilterProxyModel*>(view->model());
}
