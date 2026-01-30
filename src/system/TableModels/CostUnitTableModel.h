#pragma once
#include <QAbstractTableModel>
#include <QString>
#include <vector>
#include <unordered_map>
#include <cstdint>

#include "DatabaseDefines.h"

class CostUnitTableModel : public QAbstractTableModel
{
    Q_OBJECT

   public:
    using CostUnitMap = std::unordered_map<std::uint16_t, CostUnitInformation>;

    explicit CostUnitTableModel(QObject* parent = nullptr);

    // Data input
    void setData(const CostUnitMap& map);

    // Filtering
    void setFilterText(const QString& text);
    void setOnlyActive(bool onlyActive);

    // QAbstractTableModel interface
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

   private:
    void rebuildRows();
    QString getLocalizedName(const CostUnitInformation& info) const;

   private:
    std::vector<CostUnitInformation> _allRows;  // full dataset
    std::vector<CostUnitInformation> _rows;     // filtered / visible dataset

    QString _filterText;
    bool _onlyActive{false};
};

