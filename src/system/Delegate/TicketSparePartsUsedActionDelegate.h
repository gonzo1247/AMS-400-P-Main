#pragma once

#include <QStyledItemDelegate>
#include <cstdint>

class TicketSparePartsUsedActionDelegate final : public QStyledItemDelegate
{
    Q_OBJECT

   public:
    explicit TicketSparePartsUsedActionDelegate(std::uint32_t removePermissionId, QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option,
                     const QModelIndex& index) override;

   signals:
    void removeRequested(std::uint64_t rowId);

   private:
    std::uint64_t GetRowId(const QModelIndex& index) const;
    bool HasRemovePermission() const;

    std::uint32_t _removePermissionId{};
};
