#include "pch.h"
#include "TicketAttachmentTableModel.h"

#include <QBrush>
#include <QColor>
#include <QDateTime>

#include "UserCache.h"
#include "UserManagement.h"


TicketAttachmentTableModel::TicketAttachmentTableModel(QObject* parent) : QAbstractTableModel(parent) {}

void TicketAttachmentTableModel::setAttachments(const std::vector<TicketAttachmentInformation>& data)
{
    beginResetModel();
    _rows = data;  // copy is fine (DB loads anyway)
    endResetModel();
}

void TicketAttachmentTableModel::clear()
{
    beginResetModel();
    _rows.clear();
    endResetModel();
}

int TicketAttachmentTableModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(_rows.size());
}

int TicketAttachmentTableModel::columnCount(const QModelIndex& parent) const { return parent.isValid() ? 0 : ColCount; }

QVariant TicketAttachmentTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return {};

    const int row = index.row();
    const int col = index.column();

    if (row < 0 || row >= static_cast<int>(_rows.size()))
        return {};

    const TicketAttachmentInformation& att = _rows[row];

    if (role == Qt::DisplayRole)
    {
        switch (col)
        {
            case ColFileName:
                return QString::fromStdString(att.originalFilename);

            case ColDescription:
                return QString::fromStdString(att.description);

            case ColSize:
                return static_cast<qulonglong>(att.fileSize);

            case ColUploadedAt:
            {
                // Convert SystemTimePoint -> QDateTime
                const auto ms =
                    std::chrono::duration_cast<std::chrono::milliseconds>(att.uploadedAt.time_since_epoch()).count();

                return QDateTime::fromMSecsSinceEpoch(ms).toString("yyyy-MM-dd HH:mm");
            }

            case ColMimeType:
                return QString::fromStdString(att.mimeType);

            case ColUploaderID:
            {
                auto user = UserCache::instance().GetUserDataByID(att.uploaderUserID);
                QString name = tr("Unknown");

                if (user.has_value())
                    name = QString::fromStdString(user->GetUserLastName());

                return name;
            }
        }
    }

    if (role == Qt::BackgroundRole && att.isDeleted)
    {
        return QBrush(QColor(240, 200, 200));  // soft red
    }

    if (role == Qt::TextAlignmentRole)
    {
        if (col == ColSize || col == ColUploadedAt)
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);

        return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
    }

    return {};
}

QVariant TicketAttachmentTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return {};

    switch (section)
    {
        case ColFileName:
            return QObject::tr("File Name");
        case ColDescription:
            return QObject::tr("Description");
        case ColSize:
            return QObject::tr("Size");
        case ColUploadedAt:
            return QObject::tr("Uploaded At");
        case ColMimeType:
            return QObject::tr("Mime Type");
        case ColUploaderID:
            return QObject::tr("Uploader");
    }

    return {};
}

Qt::ItemFlags TicketAttachmentTableModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

std::optional<TicketAttachmentInformation> TicketAttachmentTableModel::getAttachment(int row) const
{
    if (row < 0 || row >= static_cast<int>(_rows.size()))
        return std::nullopt;

    return _rows[row];
}
