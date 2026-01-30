#pragma once

#include <QAbstractTableModel>
#include <QString>
#include <optional>
#include <vector>

#include "ShowTicketManager.h"

class TicketTableModel : public QAbstractTableModel
{
    Q_OBJECT

   public:
    explicit TicketTableModel(QObject* parent = nullptr);

    void setRows(const std::vector<TicketRowData>& rows);
    void setFilterText(const QString& text);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    void sort(int column, Qt::SortOrder order) override;

    std::optional<std::uint64_t> ticketIdForRow(int row) const;
    std::optional<std::uint64_t> ticketIdForIndex(const QModelIndex& index) const;

    void refreshOpenDurations();

   private:
    void rebuildRows();

   private:
    QString _filterText{};
    std::vector<TicketRowData> _allRows;
    std::vector<TicketRowData> _rows;
};
