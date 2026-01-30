#pragma once

#include <QSortFilterProxyModel>
#include <QString>

class CompanyContactProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

   public:
    explicit CompanyContactProxyModel(QObject* parent = nullptr);

    void setQuickFilterText(const QString& text);

    void setAdvancedCompanyName(const QString& text);
    void setAdvancedCustomerNumber(const QString& text);
    void setAdvancedCity(const QString& text);
    void setAdvancedPhone(const QString& text);

   protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

   private:
    QString _quick;
    QString _companyName;
    QString _customerNumber;
    QString _city;
    QString _phone;

    static bool RowContainsAny(const QString& part, const QStringList& values);
};
