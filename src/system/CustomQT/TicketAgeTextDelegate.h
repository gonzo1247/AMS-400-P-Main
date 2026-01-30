#pragma once
#include <QStyledItemDelegate>


class TicketAgeTextDelegate : public QStyledItemDelegate
{
    Q_OBJECT

   public:
    explicit TicketAgeTextDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

   private:
    static QColor ColorForMinutes(qint64 minutesOpen);
};
