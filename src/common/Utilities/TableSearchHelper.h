#pragma once

#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <QTableView>

class TableSearchHelper final
{
   public:
    static QSortFilterProxyModel* Setup(QTableView* view, QLineEdit* searchEdit, QAbstractItemModel* sourceModel,
                                        int filterColumn = -1);

    static QModelIndex MapToSource(const QTableView* view, const QModelIndex& viewIndex);
    static int MapRowToSource(const QTableView* view, int viewRow);

   private:
    static QSortFilterProxyModel* GetProxy(const QTableView* view);
};
