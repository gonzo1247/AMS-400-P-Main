#include "pch.h"
#include "CompanyContactProxyModel.h"

#include <QAbstractItemModel>

CompanyContactProxyModel::CompanyContactProxyModel(QObject* parent) : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

void CompanyContactProxyModel::setQuickFilterText(const QString& text)
{
    _quick = text.trimmed();
    invalidateFilter();
}

void CompanyContactProxyModel::setAdvancedCompanyName(const QString& text)
{
    _companyName = text.trimmed();
    invalidateFilter();
}

void CompanyContactProxyModel::setAdvancedCustomerNumber(const QString& text)
{
    _customerNumber = text.trimmed();
    invalidateFilter();
}

void CompanyContactProxyModel::setAdvancedCity(const QString& text)
{
    _city = text.trimmed();
    invalidateFilter();
}

void CompanyContactProxyModel::setAdvancedPhone(const QString& text)
{
    _phone = text.trimmed();
    invalidateFilter();
}

bool CompanyContactProxyModel::RowContainsAny(const QString& part, const QStringList& values)
{
    for (const auto& v : values)
    {
        if (v.contains(part, Qt::CaseInsensitive))
            return true;
    }

    return false;
}

bool CompanyContactProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    const auto* m = sourceModel();
    if (m == nullptr)
        return true;

    auto col = [&](int c) -> QString { return m->index(sourceRow, c, sourceParent).data(Qt::DisplayRole).toString(); };

    // Must match your Column enum order in the table model:
    const QString customerNo = col(0);
    const QString company = col(1);
    const QString contact = col(2);
    const QString mail = col(3);
    const QString phone = col(4);
    const QString street = col(5);
    const QString houseNo = col(6);
    const QString zip = col(7);
    const QString city = col(8);
    const QString country = col(9);

    // Advanced: AND
    if (!_companyName.isEmpty() && !company.contains(_companyName, Qt::CaseInsensitive))
        return false;

    if (!_customerNumber.isEmpty() && !customerNo.contains(_customerNumber, Qt::CaseInsensitive))
        return false;

    if (!_city.isEmpty() && !city.contains(_city, Qt::CaseInsensitive))
        return false;

    if (!_phone.isEmpty() && !phone.contains(_phone, Qt::CaseInsensitive))
        return false;

    // Quick: space-split, each part must be found in any column (AND over parts, OR over columns)
    if (_quick.isEmpty())
        return true;

    const QStringList values = {customerNo, company, contact, mail, phone, street, houseNo, zip, city, country};
    const QStringList parts = _quick.split(' ', Qt::SkipEmptyParts);

    for (const auto& part : parts)
    {
        if (!RowContainsAny(part, values))
            return false;
    }

    return true;
}
