#pragma once

#include <QStyledItemDelegate>

class TicketColorDelegate : public QStyledItemDelegate
{
    Q_OBJECT

   public:
    explicit TicketColorDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    static QColor ColorForMinutes(qint64 minutesOpen);
};
