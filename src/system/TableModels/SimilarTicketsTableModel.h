#pragma once

#include <QAbstractTableModel>
#include <QString>
#include <vector>

#include "SimilarReportManager.h"

class SimilarTicketsTableModel : public QAbstractTableModel
{
    Q_OBJECT

   public:
    enum Columns
    {
        ColumnTicketId = 0,
        ColumnTitle,
        ColumnUpdatedAt,
        ColumnScore,
        ColumnCount
    };

   public:
    explicit SimilarTicketsTableModel(QObject* parent = nullptr);

    void SetResults(std::vector<SimilarReportResult>&& results);
    const SimilarReportResult* GetRow(int row) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

   private:
    std::vector<SimilarReportResult> _rows;
};
