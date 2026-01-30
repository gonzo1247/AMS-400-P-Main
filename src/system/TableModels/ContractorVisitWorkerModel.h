#pragma once

#include <QAbstractTableModel>
#include <cstdint>
#include <string>
#include <vector>

struct ContractorVisitWorkerInfo
{
    std::uint64_t workerId = 0;
    std::string firstName;
    std::string lastName;
    std::string phone;
    std::string company;
};

class ContractorVisitWorkerModel final : public QAbstractTableModel
{
    Q_OBJECT

   public:
    enum Column : int
    {
        COL_FIRST_NAME = 0,
        COL_LAST_NAME = 1,
        COL_PHONE = 2,
        COL_COMPANY = 3,

        COL_COUNT
    };

    static constexpr int RoleWorkerId = Qt::UserRole + 1;

    explicit ContractorVisitWorkerModel(QObject* parent = nullptr);

    void setRows(const std::vector<ContractorVisitWorkerInfo>& rows);
    void clear();

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    std::uint64_t getWorkerId(int row) const;

   private:
    std::vector<ContractorVisitWorkerInfo> _rows;
};
