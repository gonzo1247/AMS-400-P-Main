#pragma once

#include <QAbstractTableModel>
#include <QSet>
#include <QString>
#include <vector>

#include "SharedDefines.h"


class QTableView;

class CompanyContactTableModel : public QAbstractTableModel
{
    Q_OBJECT

   public:
    enum class Column : int
    {
        CustomerNumber = 0,
        CompanyName,
        ContactPerson,
        MailAddress,
        PhoneNumber,
        Street,
        HouseNumber,
        PostalCode,
        City,
        Country,
        ResponsibleFor,
        Website,
        MoreInformation,
        Highlight,

        Count
    };

    explicit CompanyContactTableModel(QObject* parent = nullptr);

    void setDataRows(std::vector<CompanyContactEntry> rows);

    const CompanyContactEntry* rowAt(int row) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void sort(int column, Qt::SortOrder order) override;

    void setGrantedPermissions(const QSet<int>& permissions);
    bool isColumnAllowed(int column) const;

    void applyColumnVisibility(QTableView* view) const;

   private:
    std::vector<CompanyContactEntry> _rows;
    QSet<int> _grantedPermissions;

    QString columnText(const CompanyContactEntry& e, Column c) const;

    int requiredPermissionForColumn(Column c) const;
};
