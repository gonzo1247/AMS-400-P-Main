#pragma once

#include <QAbstractListModel>
#include <cstdint>
#include <string>
#include <vector>


struct ContractorVisitStatusData;

class ContractorVisitStatusModel final : public QAbstractListModel
{
    Q_OBJECT

   public:
    enum Roles
    {
        IdRole = Qt::UserRole + 1,
        DescriptionRole
    };

    explicit ContractorVisitStatusModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;

    void setRows(std::vector<ContractorVisitStatusData> rows);
    void clear();

   private:
    std::vector<ContractorVisitStatusData> _rows;
};
