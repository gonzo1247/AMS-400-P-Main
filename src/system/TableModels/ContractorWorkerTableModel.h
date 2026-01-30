#pragma once

#include <QAbstractTableModel>
#include <cstdint>
#include <vector>

struct ContractorWorkerDataDB;

class ContractorWorkerTableModel : public QAbstractTableModel
{
    Q_OBJECT

   public:
    enum class Column : int
    {
        CompanyName = 0,
        FirstName,
        LastName,
        Phone,
        Email,
        Note,
        Active,

        Count
    };

    explicit ContractorWorkerTableModel(QObject* parent = nullptr);

    void setDataRows(std::vector<ContractorWorkerDataDB> rows);

    std::uint32_t getWorkerId(int row) const;
    const ContractorWorkerDataDB* getWorker(int row) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void sort(int column, Qt::SortOrder order) override;

   private:
    std::vector<ContractorWorkerDataDB> _rows;

    QString columnText(const ContractorWorkerDataDB& e, Column col) const;
};
