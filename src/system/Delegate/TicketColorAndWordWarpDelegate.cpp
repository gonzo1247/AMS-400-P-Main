#include "TicketColorAndWordWarpDelegate.h"

#include <QApplication>
#include <QPainter>
#include <QTextDocument>
#include <cmath>
#include <QAbstractTextDocumentLayout>

#include "ShowTicketDetailManager.h"
#include "pch.h"

namespace
{
    constexpr int RoleMinutesOpen = Qt::UserRole + 1000;
    constexpr int RoleStatusPriority = Qt::UserRole + 1;

    constexpr int ColPriority = 5;
    constexpr int ColStatus = 6;
    constexpr int ColOpenFor = 13;

    QColor ColorForMinutesLocal(qint64 minutesOpen)
    {
        if (minutesOpen >= 48 * 60)
            return QColor(220, 60, 60, 150);

        if (minutesOpen >= 12 * 60)
            return QColor(255, 165, 79, 150);

        if (minutesOpen >= 4 * 60)
            return QColor(255, 255, 153, 150);

        return QColor(200, 255, 200, 150);
    }

    void ApplyTicketColorFormatting(QStyleOptionViewItem* opt, const QModelIndex& index)
    {
        if (opt == nullptr)
            return;

        if (opt->state & QStyle::State_Selected)
            return;

        if (index.column() == ColStatus)
        {
            const QVariant v = index.data(RoleStatusPriority);
            if (v.isValid() && v.canConvert<int>())
                opt->backgroundBrush = QBrush(GetTicketStatusColor(static_cast<TicketStatus>(v.toInt())));
        }
        else if (index.column() == ColPriority)
        {
            const QVariant v = index.data(RoleStatusPriority);
            if (v.isValid() && v.canConvert<int>())
                opt->backgroundBrush = QBrush(GetTicketPriorityColor(static_cast<TicketPriority>(v.toInt())));
        }
        else if (index.column() == ColOpenFor)
        {
            const QVariant v = index.data(RoleMinutesOpen);
            if (v.isValid())
            {
                const qint64 minutesOpen = v.toLongLong();
                opt->backgroundBrush = QBrush(ColorForMinutesLocal(minutesOpen));

                opt->palette.setColor(QPalette::Text, Qt::black);
                opt->palette.setColor(QPalette::HighlightedText, Qt::black);

                if (minutesOpen >= 48 * 60)
                    opt->font.setBold(true);
            }
        }
    }
}  // namespace

TicketColorAndWordWarpDelegate::TicketColorAndWordWarpDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

QColor TicketColorAndWordWarpDelegate::ColorForMinutes(qint64 minutesOpen) { return ColorForMinutesLocal(minutesOpen); }

void TicketColorAndWordWarpDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                           const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    ApplyTicketColorFormatting(&opt, index);

    const QWidget* w = opt.widget;
    QStyle* style = w ? w->style() : QApplication::style();

    painter->save();

    if (!(opt.state & QStyle::State_Selected) && opt.backgroundBrush.style() != Qt::NoBrush)
        painter->fillRect(opt.rect, opt.backgroundBrush);

    const QString text = opt.text;
    opt.text.clear();

    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, w);

    QTextDocument doc;
    doc.setDefaultFont(opt.font);
    doc.setDocumentMargin(0.0);
    doc.setTextWidth(opt.rect.width());
    doc.setPlainText(text);

    QTextOption textOpt = doc.defaultTextOption();
    textOpt.setAlignment(Qt::AlignHCenter);
    textOpt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    doc.setDefaultTextOption(textOpt);

    const qreal docHeight = doc.size().height();
    const qreal yOffset = std::max<qreal>(0.0, (opt.rect.height() - docHeight) * 0.5);

    painter->translate(opt.rect.left(), opt.rect.top() + yOffset);

    const QRectF clip(0.0, 0.0, opt.rect.width(), opt.rect.height() - yOffset);
    doc.drawContents(painter, clip);

    painter->restore();
}

QSize TicketColorAndWordWarpDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    ApplyTicketColorFormatting(&opt, index);

    QTextDocument doc;
    doc.setDefaultFont(opt.font);
    doc.setTextWidth(opt.rect.width());
    doc.setPlainText(opt.text);

    const QSizeF s = doc.size();
    return QSize(static_cast<int>(s.width()), static_cast<int>(std::ceil(s.height())));
}
