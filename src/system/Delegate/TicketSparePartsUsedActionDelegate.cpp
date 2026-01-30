#include "pch.h"
#include "TicketSparePartsUsedActionDelegate.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionButton>

#include "RBACAccess.h"
#include "TicketSparePartsUsedModel.h"

TicketSparePartsUsedActionDelegate::TicketSparePartsUsedActionDelegate(std::uint32_t removePermissionId, QObject* parent) : QStyledItemDelegate(parent), _removePermissionId(removePermissionId)
{
}

bool TicketSparePartsUsedActionDelegate::HasRemovePermission() const
{
    if (_removePermissionId == 0)
    {
        return true;
    }

    return RBACAccess::HasPermission(_removePermissionId);
}

std::uint64_t TicketSparePartsUsedActionDelegate::GetRowId(const QModelIndex& index) const
{
    const auto* m = qobject_cast<const TicketSparePartsUsedModel*>(index.model());
    if (!m)
    {
        return 0;
    }

    if (index.row() < 0 || index.row() >= m->rowCount())
    {
        return 0;
    }

    return m->GetRow(index.row()).spareData.id;
}

void TicketSparePartsUsedActionDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    painter->save();

    const bool canRemove = HasRemovePermission();

    QStyleOptionButton btn;
    btn.rect = opt.rect.adjusted(6, 4, -6, -4);
    btn.text = QObject::tr("Remove");
    btn.state = QStyle::State_Enabled;

    if (!canRemove)
    {
        btn.state &= ~QStyle::State_Enabled;
    }

    QApplication::style()->drawControl(QStyle::CE_PushButton, &btn, painter);

    painter->restore();
}

bool TicketSparePartsUsedActionDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    (void)model;

    if (!index.isValid())
    {
        return false;
    }

    if (!HasRemovePermission())
    {
        return false;
    }

    if (event->type() != QEvent::MouseButtonRelease)
    {
        return false;
    }

    const auto* mouse = static_cast<QMouseEvent*>(event);
    if (mouse->button() != Qt::LeftButton)
    {
        return false;
    }

    const QRect buttonRect = option.rect.adjusted(6, 4, -6, -4);
    if (!buttonRect.contains(mouse->pos()))
    {
        return false;
    }

    const std::uint64_t rowId = GetRowId(index);
    if (rowId == 0)
    {
        return false;
    }

    emit removeRequested(rowId);
    return true;
}
