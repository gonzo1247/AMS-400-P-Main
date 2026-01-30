#include "pch.h"
#include "CompanyContactTableModel.h"

#include <QBrush>
#include <QTableView>
#include <algorithm>

CompanyContactTableModel::CompanyContactTableModel(QObject* parent) : QAbstractTableModel(parent) {}

void CompanyContactTableModel::setDataRows(std::vector<CompanyContactEntry> rows)
{
    beginResetModel();
    _rows = std::move(rows);
    endResetModel();
}

const CompanyContactEntry* CompanyContactTableModel::rowAt(int row) const
{
    if (row < 0 || row >= static_cast<int>(_rows.size()))
        return nullptr;

    return &_rows[static_cast<size_t>(row)];
}

int CompanyContactTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return static_cast<int>(_rows.size());
}

int CompanyContactTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return static_cast<int>(Column::Count);
}

QVariant CompanyContactTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return {};

    const int r = index.row();
    const int c = index.column();

    if (r < 0 || r >= rowCount() || c < 0 || c >= columnCount())
        return {};

    if (!isColumnAllowed(c) && role == Qt::DisplayRole)
        return {};

    const auto& e = _rows[static_cast<size_t>(r)];
    const auto col = static_cast<Column>(c);

    if (role == Qt::DisplayRole)
        return columnText(e, col);

    if (role == Qt::ToolTipRole)
        return columnText(e, col);

    if (role == Qt::TextAlignmentRole)
    {
        switch (col)
        {
            case Column::CustomerNumber:
            case Column::PostalCode:
                return static_cast<int>(Qt::AlignVCenter | Qt::AlignRight);

            case Column::Highlight:
                return Qt::AlignCenter;

            default:
                return static_cast<int>(Qt::AlignVCenter | Qt::AlignLeft);
        }
    }

    if (role == Qt::ForegroundRole)
    {
        if (e.isDeleted != 0)
            return QBrush(Qt::gray);
    }

    return {};
}

QVariant CompanyContactTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractTableModel::headerData(section, orientation, role);

    const auto col = static_cast<Column>(section);

    switch (col)
    {
        case Column::CustomerNumber:
            return tr("Customer No.");
        case Column::CompanyName:
            return tr("Company");
        case Column::ContactPerson:
            return tr("Contact");
        case Column::MailAddress:
            return tr("Mail");
        case Column::PhoneNumber:
            return tr("Phone");
        case Column::Street:
            return tr("Street");
        case Column::HouseNumber:
            return tr("No.");
        case Column::PostalCode:
            return tr("ZIP");
        case Column::City:
            return tr("City");
        case Column::Country:
            return tr("Country");
        case Column::ResponsibleFor:
            return tr("Responsible for");
        case Column::Website:
            return tr("Website");
        case Column::MoreInformation:
            return tr("More info");
        case Column::Highlight:
            return tr("★");
        default:
            return {};
    }
}

QString CompanyContactTableModel::columnText(const CompanyContactEntry& e, Column c) const
{
    switch (c)
    {
        case Column::CustomerNumber:
            return e.customerNumber;
        case Column::CompanyName:
            return e.companyName;
        case Column::ContactPerson:
            return e.contactPerson;
        case Column::MailAddress:
            return e.mailAddress;
        case Column::PhoneNumber:
            return e.phoneNumber;
        case Column::Street:
            return e.street;
        case Column::HouseNumber:
            return e.houseNumber;
        case Column::PostalCode:
            return e.postalCode;
        case Column::City:
            return e.city;
        case Column::Country:
            return e.country;
        case Column::ResponsibleFor:
            return e.responsibleFor;
        case Column::Website:
            return e.website;
        case Column::MoreInformation:
            return e.moreInformation;
        case Column::Highlight:
            return (e.highlight != 0) ? QStringLiteral("1") : QString();
        default:
            return {};
    }
}

void CompanyContactTableModel::sort(int column, Qt::SortOrder order)
{
    if (column < 0 || column >= columnCount())
        return;

    const auto col = static_cast<Column>(column);

    beginResetModel();

    std::sort(_rows.begin(), _rows.end(),
              [&](const CompanyContactEntry& a, const CompanyContactEntry& b)
              {
                  const QString av = columnText(a, col);
                  const QString bv = columnText(b, col);

                  const int cmp = QString::compare(av, bv, Qt::CaseInsensitive);
                  return (order == Qt::AscendingOrder) ? (cmp < 0) : (cmp > 0);
              });

    endResetModel();
}

void CompanyContactTableModel::setGrantedPermissions(const QSet<int>& permissions)
{
    _grantedPermissions = permissions;
}

int CompanyContactTableModel::requiredPermissionForColumn(Column c) const
{
    // Use your RBAC permission IDs here.
    // Return -1 if always allowed.

    switch (c)
    {
        case Column::MailAddress:
            return /* RBAC_VIEW_CONTACT_MAIL */ -1;
        case Column::PhoneNumber:
            return /* RBAC_VIEW_CONTACT_PHONE */ -1;
/*
        case Column::Address:
            return / * RBAC_VIEW_CONTACT_ADDRESS * / -1;*/
        case Column::Street:
            return /* RBAC_VIEW_CONTACT_ADDRESS */ -1;
        case Column::HouseNumber:
            return /* RBAC_VIEW_CONTACT_ADDRESS */ -1;
        case Column::PostalCode:
            return /* RBAC_VIEW_CONTACT_ADDRESS */ -1;
        default:
            return -1;
    }
}

bool CompanyContactTableModel::isColumnAllowed(int column) const
{
    const auto col = static_cast<Column>(column);
    const int perm = requiredPermissionForColumn(col);

    if (perm < 0)
        return true;

    return _grantedPermissions.contains(perm);
}

void CompanyContactTableModel::applyColumnVisibility(QTableView* view) const
{
    if (view == nullptr)
        return;

    for (int c = 0; c < columnCount(); ++c)
        view->setColumnHidden(c, !isColumnAllowed(c));
}
