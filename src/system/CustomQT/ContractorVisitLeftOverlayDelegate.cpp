#include "pch.h"

#include <QPainter>

#include "ContractorVisitLeftOverlayDelegate.h"

#include "ContractorVisitModel.h"
#include "Logger.h"
#include "LoggerDefines.h"
#include "pch.h"

ContractorVisitLeftOverlayDelegate::ContractorVisitLeftOverlayDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

void ContractorVisitLeftOverlayDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    const QVariant v = index.data(ContractorVisitModel::RoleHasLeft);
    const bool hasLeft = v.isValid() && v.toBool();

    LOG_DEBUG("Contractor visit has left value at row {}: valid={} value={}", index.row(), v.isValid(), hasLeft);

    if (hasLeft && !(opt.state & QStyle::State_Selected))
    {
        painter->save();
        painter->fillRect(opt.rect, QColor(60, 60, 60, 40));
        painter->restore();

        LOG_DEBUG("Contractor visit has left overlay applied at row {}", index.row());

        opt.palette.setColor(QPalette::Text, QColor(120, 120, 120));
        opt.palette.setColor(QPalette::HighlightedText, QColor(120, 120, 120));
    }

    QStyledItemDelegate::paint(painter, opt, index);
}
