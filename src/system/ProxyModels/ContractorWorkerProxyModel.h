#pragma once

#include <QSortFilterProxyModel>
#include <QString>

class ContractorWorkerProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

   public:
    explicit ContractorWorkerProxyModel(QObject* parent = nullptr);

    void setQuickFilterText(const QString& text);
    void setOnlyActive(bool onlyActive);

   protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

   private:
    QString _quick;
    bool _onlyActive = false;

    static bool RowContainsAny(const QString& part, const QStringList& values);
};
