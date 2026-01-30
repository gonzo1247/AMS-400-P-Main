#include "pch.h"
#include "ContractorVisitModel.h"


#include <QRegularExpression>
#include <chrono>

#include "ContractorVisitLeftOverlayDelegate.h"
#include "ContractorVisitStatusManager.h"
#include "Util.h"
#include "pch.h"

ContractorVisitModel::ContractorVisitModel(ViewMode mode, QObject* parent) : QAbstractTableModel(parent), _mode(mode) {}

void ContractorVisitModel::setRows(const std::vector<ContractorVisitInformation>& rows)
{
    beginResetModel();
    _rows = rows;
    endResetModel();
}

int ContractorVisitModel::rowCount(const QModelIndex&) const
{
    return static_cast<int>(_rows.size());
}

int ContractorVisitModel::columnCount(const QModelIndex&) const
{
    return COL_COUNT;
}

qint64 ContractorVisitModel::CalcMinutesOnSite(const ContractorVisitInformation& row) const
{
    const auto now = Util::GetCurrentSystemPointTime();

    if (now <= row.arrival_at)
        return 0;

    // Already left
    if (row.departure_at > row.arrival_at)
    {
        const auto diffSec =
            std::chrono::duration_cast<std::chrono::seconds>(row.departure_at - row.arrival_at).count();
        return static_cast<qint64>(diffSec / 60);
    }

    // Still on site
    const auto diffSec = std::chrono::duration_cast<std::chrono::seconds>(now - row.arrival_at).count();
    return static_cast<qint64>(diffSec / 60);
}

QVariant ContractorVisitModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || static_cast<std::size_t>(index.row()) >= _rows.size())
        return {};

    const auto& row = _rows[index.row()];

    if (role == RoleVisitId)
        return static_cast<qulonglong>(row.id);

    if (role == RoleStatus)
        return static_cast<int>(row.status);

    if (role == RoleMinutesOnSite)
        return CalcMinutesOnSite(row);

    if (role == RoleHasLeft)
        return row.departure_at > row.arrival_at;

    if (role != Qt::DisplayRole)
        return {};

    switch (index.column())
    {
        case COL_ID:
            return static_cast<qulonglong>(row.id);

        case COL_COMPANY_NAME:
            return QString::fromStdString(row.contractorCompany);

        case COL_COMPANY_EXTERNAL_ID:
            return static_cast<qulonglong>(row.contractorCompanyID);

        case COL_CONTACT_PERSON:
            return QString::fromStdString(row.contactPerson);

        case COL_WORKER_NAME:
            return QString::fromStdString(row.workerName);

        case COL_ARRIVAL_AT:
            return Util::FormatDateTimeQString(row.arrival_at);

        case COL_DEPARTURE_AT:
        {
            if (row.departure_at > row.arrival_at)
                return Util::FormatDateTimeQString(row.departure_at);

            return QString();
        }

        case COL_ACTIVITY:
            return QString::fromStdString(row.activity);

        case COL_LOCATION:
            return QString::fromStdString(row.location);

        case COL_REACHABLE_PHONE:
            return QString::fromStdString(row.contactPhone);

        case COL_REACHABLE_NOTE:
            return QString::fromStdString(row.contactNote);

        case COL_STATUS:
            return QString::fromStdString(ContractorVisitStatusManager::GetName(row.status));

        case COL_NOTE:
            return QString::fromStdString(row.note);

        case COL_CREATED_BY_USER_ID:
            return static_cast<qulonglong>(row.creatorUserID);

        case COL_CREATED_AT:
            return Util::FormatDateTimeQString(row.createdAt);

        case COL_UPDATED_AT:
            return Util::FormatDateTimeQString(row.updatedAt);

        default:
            return {};
    }
}

QVariant ContractorVisitModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section)
    {
        case COL_ID:
            return tr("ID");
        case COL_COMPANY_NAME:
            return tr("Company");
        case COL_COMPANY_EXTERNAL_ID:
            return tr("Company ID");
        case COL_CONTACT_PERSON:
            return tr("Contact");
        case COL_WORKER_NAME:
            return tr("Worker");
        case COL_ARRIVAL_AT:
            return tr("Arrival");
        case COL_DEPARTURE_AT:
            return tr("Departure");
        case COL_ACTIVITY:
            return tr("Activity");
        case COL_LOCATION:
            return tr("Location");
        case COL_REACHABLE_PHONE:
            return tr("Phone");
        case COL_REACHABLE_NOTE:
            return tr("Reachable Note");
        case COL_STATUS:
            return tr("Status");
        case COL_NOTE:
            return tr("Note");
        case COL_CREATED_BY_USER_ID:
            return tr("Creator");
        case COL_CREATED_AT:
            return tr("Created");
        case COL_UPDATED_AT:
            return tr("Updated");
        default:
            return {};
    }
}

void ContractorVisitModel::refreshLiveFields()
{
    if (_mode != ViewMode::Dashboard || _rows.empty())
        return;

    const QModelIndex tl = this->index(0, 0);
    const QModelIndex br = this->index(rowCount() - 1, columnCount() - 1);

    emit dataChanged(tl, br, {RoleMinutesOnSite});
}
