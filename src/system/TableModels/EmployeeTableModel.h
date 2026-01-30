#pragma once

#include <QAbstractTableModel>
#include <cstdint>
#include <string>
#include <vector>

#include "DatabaseDefines.h"

// Adjust this alias to your actual map type
using EmployeeMap = std::unordered_map<std::uint32_t, EmployeeInformation>;

class EmployeeTableModel final : public QAbstractTableModel
{
    Q_OBJECT

   public:
    explicit EmployeeTableModel(QObject* parent = nullptr);

    void setData(const EmployeeMap& map);
    void setFilterText(const QString& text);
    void setOnlyActive(bool onlyActive);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void sort(int column, Qt::SortOrder order) override;

   private:
    void rebuildRows();

   private:
    std::vector<EmployeeInformation> _allRows;
    std::vector<EmployeeInformation> _rows;
    QString _filterText;
    bool _onlyActive{false};
};
