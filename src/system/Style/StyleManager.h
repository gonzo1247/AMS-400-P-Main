#pragma once
#include <QTableView>

#include "TicketTableModel.h"

class StyleManager
{
public:
    StyleManager();
    ~StyleManager() = default;

    void SetOpenForColor(QTableView* table, const TicketTableModel& model, const QColor& color, int& row);
};
