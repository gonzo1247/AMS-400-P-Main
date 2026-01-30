#pragma once

#include <QStyledItemDelegate>

class ContractorVisitLeftOverlayDelegate final : public QStyledItemDelegate
{
    Q_OBJECT

   public:

    explicit ContractorVisitLeftOverlayDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};
