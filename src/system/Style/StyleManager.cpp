#include "pch.h"
#include "StyleManager.h"

#include "SimpleBGDelegate.h"

StyleManager::StyleManager() {}

void StyleManager::SetOpenForColor(QTableView* table, const TicketTableModel& model, const QColor& color, int& row)
 {
    QAbstractItemView* view = table;
    view->setItemDelegate(new SimpleBgDelegate(view));

 //   model.setBackgroundColor(row, 13, color);
 }
