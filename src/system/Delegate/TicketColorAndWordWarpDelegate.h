#pragma once

#include <QStyledItemDelegate>

class TicketColorAndWordWarpDelegate : public QStyledItemDelegate
{
    Q_OBJECT

   public:
    explicit TicketColorAndWordWarpDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

   private:
    static QColor ColorForMinutes(qint64 minutesOpen);
};
