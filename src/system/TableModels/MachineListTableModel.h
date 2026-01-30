#pragma once

#include <QAbstractTableModel>
#include <QString>
#include <cstdint>
#include <string>
#include <vector>

#include "DatabaseDefines.h"

class CompanyLocationHandler;

class MachineListTableModel : public QAbstractTableModel
{
    Q_OBJECT

   public:
    explicit MachineListTableModel(QObject* parent = nullptr);

    // Load full machine list
    void setData(const std::vector<MachineInformation>& machines);

    // Simple text filter (name, numbers, info, ids)
    void setFilterText(const QString& text);

    // Filter only active machines
    void setOnlyActive(bool onlyActive);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void sort(int column, Qt::SortOrder order) override;

    bool IsVisibleByCurrentFilter(const MachineInformation& info) const;
    bool ApplyMachinePatch(const MachineInformation& updated);

   private:
    void rebuildRows();
    void rebuildVisibleRowsNoReset();

   private:
    std::vector<MachineInformation> _allRows;
    std::vector<MachineInformation> _rows;
 
    std::unordered_map<std::uint32_t, std::size_t> _allIndexById;
    std::unordered_map<std::uint32_t, std::size_t> _visibleIndexById;

    int _lastSortColumn = -1;
    Qt::SortOrder _lastSortOrder = Qt::AscendingOrder;

    QString _filterText;
    bool _onlyActive{false};
};
