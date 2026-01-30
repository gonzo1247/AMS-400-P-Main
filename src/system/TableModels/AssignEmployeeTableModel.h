#pragma once
#include "DatabaseDefines.h"

class AssignEmployeeTableModel : public QAbstractTableModel
{
    Q_OBJECT

   public:
    explicit AssignEmployeeTableModel(QObject* parent = nullptr);

    void setRows(const std::vector<EmployeeInformation>& rows);
    void setFilterText(const QString& text);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void sort(int column, Qt::SortOrder order) override;

    std::optional<std::uint32_t> employeeIdForRow(int row) const;
    std::optional<std::uint32_t> employeeIdForIndex(const QModelIndex& index) const;

    void setEmployeeActiveByIndex(const QModelIndex& index, bool active);
    void setEmployeeActiveById(std::uint32_t employeeId, bool active);

    bool isEmployeeActiveByIndex(const QModelIndex& index) const;

   private:
    void rebuildRows();

   private:
    std::vector<EmployeeInformation> _allRows;
    std::vector<EmployeeInformation> _rows;
    QString _filterText;
};
