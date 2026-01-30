#include "TicketAgeTextDelegate.h"

#include <QPainter>

#include "pch.h"

TicketAgeTextDelegate::TicketAgeTextDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

QColor TicketAgeTextDelegate::ColorForMinutes(qint64 minutesOpen)
{
    if (minutesOpen >= 48 * 60)
        return QColor(220, 60, 60, 150);  // soft red

    if (minutesOpen >= 12 * 60)
        return QColor(255, 165, 79, 150);  // warm orange

    if (minutesOpen >= 4 * 60)
        return QColor(255, 255, 153, 150);  // soft yellow

    return QColor(200, 255, 200, 150);  // light green
}

void TicketAgeTextDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    if (index.column() != 13)
    {
        QStyledItemDelegate::paint(painter, opt, index);
        return;
    }

    if (!(opt.state & QStyle::State_Selected))
    {
        const QVariant v = index.data(Qt::UserRole + 1000);  // minutesOpen
        if (v.isValid())
        {
            const qint64 minutesOpen = v.toLongLong();

            painter->save();
            painter->fillRect(opt.rect, ColorForMinutes(minutesOpen));
            painter->restore();

            if (minutesOpen >= 48 * 60)
                opt.font.setBold(true);
        }
    }

    QStyledItemDelegate::paint(painter, opt, index);
}
