#pragma once

#include <QStyledItemDelegate>

class WordWrapItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

   public:
    explicit WordWrapItemDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};
