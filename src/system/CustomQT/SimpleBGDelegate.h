#pragma once
#include <QStyledItemDelegate>


class SimpleBgDelegate : public QStyledItemDelegate
{
   public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* p, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        QVariant v = index.data(Qt::BackgroundRole);
        if (v.isValid() && !(opt.state & QStyle::State_Selected))
        {
            QBrush b = v.canConvert<QBrush>() ? qvariant_cast<QBrush>(v) : QBrush(qvariant_cast<QColor>(v));
            if (b.style() != Qt::NoBrush)
            {
                p->save();
                p->fillRect(opt.rect, b);  // fill cell
                p->restore();
                opt.backgroundBrush = Qt::NoBrush;  // prevent style repaint
            }
        }

        QStyledItemDelegate::paint(p, opt, index);
    }
};
