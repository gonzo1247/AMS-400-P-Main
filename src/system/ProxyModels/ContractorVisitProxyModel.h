#pragma once

#include <QSortFilterProxyModel>
#include <QString>

class ContractorVisitProxyModel final : public QSortFilterProxyModel
{
    Q_OBJECT

   public:
    explicit ContractorVisitProxyModel(QObject* parent = nullptr);

    void setQuickFilterText(const QString& text);

   protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

   private:
    QString _quick;
};
