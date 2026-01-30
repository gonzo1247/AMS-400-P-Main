#include "pch.h"

#include <QPainter>

#include "ShowTicketDetailManager.h"
#include "TicketColorDelegate.h"

namespace
{
    constexpr int RoleMinutesOpen = Qt::UserRole + 1000;
    constexpr int RoleStatusPriority = Qt::UserRole + 1;

    constexpr int ColPriority = 5;
    constexpr int ColStatus = 6;
    constexpr int ColOpenFor = 13;
}  // namespace

TicketColorDelegate::TicketColorDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

QColor TicketColorDelegate::ColorForMinutes(qint64 minutesOpen)
{
    if (minutesOpen >= 48 * 60)
        return QColor(220, 60, 60, 150);

    if (minutesOpen >= 12 * 60)
        return QColor(255, 165, 79, 150);

    if (minutesOpen >= 4 * 60)
        return QColor(255, 255, 153, 150);

    return QColor(200, 255, 200, 150);
}

void TicketColorDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    if (!(opt.state & QStyle::State_Selected))
    {
        QColor bgColor;

        if (index.column() == ColStatus && index.row() == 0)
        {
            const QVariant v = index.data(RoleStatusPriority);
        }


        if (index.column() == ColStatus)
        {
            const QVariant v = index.data(RoleStatusPriority);
            if (v.isValid() && v.canConvert<int>())
            {
                bgColor = GetTicketStatusColor(static_cast<TicketStatus>(v.toInt()));
            }
        }
        else if (index.column() == ColPriority)
        {
            const QVariant v = index.data(RoleStatusPriority);
            if (v.isValid() && v.canConvert<int>())
                bgColor = GetTicketPriorityColor(static_cast<TicketPriority>(v.toInt()));
        }
        else if (index.column() == ColOpenFor)
        {
            const QVariant v = index.data(RoleMinutesOpen);
            if (v.isValid())
            {
                const qint64 minutesOpen = v.toLongLong();
                bgColor = ColorForMinutes(minutesOpen);
                          
/*
                painter->save();
                painter->fillRect(opt.rect, ColorForMinutes(minutesOpen));
                painter->restore();*/

                opt.palette.setColor(QPalette::Text, Qt::black);
                opt.palette.setColor(QPalette::HighlightedText, Qt::black);

                if (minutesOpen >= 48 * 60)
                    opt.font.setBold(true);
            }
        }

/*
        if (bgColor.isValid())
            opt.backgroundBrush = QBrush(bgColor);*/

        painter->save();
        painter->fillRect(opt.rect, bgColor);
        painter->restore();


    }

    QStyledItemDelegate::paint(painter, opt, index);
}
