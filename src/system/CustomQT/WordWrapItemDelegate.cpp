#include "pch.h"
#include <QAbstractItemView>
#include <QPainter>
#include <QTextDocument>

#include "WordWrapItemDelegate.h"

WordWrapItemDelegate::WordWrapItemDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

void WordWrapItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    const QWidget* w = opt.widget;
    QStyle* style = w ? w->style() : QApplication::style();

    painter->save();

    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, w);

    QTextDocument doc;
    doc.setDefaultFont(opt.font);
    doc.setTextWidth(opt.rect.width());
    doc.setPlainText(opt.text);

    opt.text.clear();

    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, w);

    painter->translate(opt.rect.topLeft());
    QRect clip(0, 0, opt.rect.width(), opt.rect.height());
    doc.drawContents(painter, clip);

    painter->restore();
}

QSize WordWrapItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QTextDocument doc;
    doc.setDefaultFont(opt.font);
    doc.setTextWidth(opt.rect.width());
    doc.setPlainText(opt.text);

    const QSizeF s = doc.size();
    return QSize(static_cast<int>(s.width()), static_cast<int>(std::ceil(s.height())));
}
