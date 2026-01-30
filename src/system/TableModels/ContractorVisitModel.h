#pragma once
#include <QAbstractTableModel>  

#include "DatabaseDefines.h"


class ContractorVisitModel final : public QAbstractTableModel
{
    Q_OBJECT

   public:
    enum Column : int
    {
        COL_ID                      = 0,
        COL_COMPANY_NAME            = 1,
        COL_COMPANY_EXTERNAL_ID     = 2,
        COL_CONTACT_PERSON          = 3,
        COL_WORKER_NAME             = 4,
        COL_ARRIVAL_AT              = 5,
        COL_DEPARTURE_AT            = 6,
        COL_ACTIVITY                = 7,
        COL_LOCATION                = 8,
        COL_REACHABLE_PHONE         = 9,
        COL_REACHABLE_NOTE          = 10,
        COL_STATUS                  = 11,
        COL_NOTE                    = 12,
        COL_CREATED_BY_USER_ID      = 13,
        COL_CREATED_AT              = 14,
        COL_UPDATED_AT              = 15,

        COL_COUNT                   = 16
    };

    enum class ViewMode
    {
        Admin,
        Dashboard
    };

    static constexpr int RoleVisitId = Qt::UserRole;
    static constexpr int RoleStatus = Qt::UserRole + 1;
    static constexpr int RoleMinutesOnSite = Qt::UserRole + 2;
    static constexpr int RoleHasLeft = Qt::UserRole + 3;


    explicit ContractorVisitModel(ViewMode mode = ViewMode::Admin, QObject* parent = nullptr);

    void setRows(const std::vector<ContractorVisitInformation>& rows);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void refreshLiveFields();

   private:
    qint64 CalcMinutesOnSite(const ContractorVisitInformation& row) const;

    std::vector<ContractorVisitInformation> _rows;

    ViewMode _mode = ViewMode::Admin;
};
