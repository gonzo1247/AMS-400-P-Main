#pragma once

#include <QAbstractTableModel>
#include <optional>
#include <vector>

#include "DatabaseDefines.h"

class TicketAttachmentTableModel : public QAbstractTableModel
{
    Q_OBJECT

   public:
    explicit TicketAttachmentTableModel(QObject* parent = nullptr);

    void setAttachments(const std::vector<TicketAttachmentInformation>& data);
    void clear();

    std::optional<TicketAttachmentInformation> getAttachment(int row) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

   private:
    enum Column
    {
        ColFileName = 0,
        ColDescription,
        ColSize,
        ColUploadedAt,
        ColMimeType,
        ColUploaderID,
        ColCount
    };

    std::vector<TicketAttachmentInformation> _rows;
};
