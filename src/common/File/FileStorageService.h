#pragma once

#include <QByteArray>
#include <QString>
#include <optional>

#include "SettingsManager.h"

class FileStorageService
{
   public:
    struct StoredFileInfo
    {
        // e.g. "TICKET_0000006/test_001.pdf"
        QString relativePath;
        // e.g. "test_001.pdf"
        QString storedFileName;
        // size in bytes
        quint64 fileSize{0};
    };

    using ProgressCallback = std::function<void(qint64, qint64)>;
    using CancelCheckCallback = std::function<bool()>;

    FileStorageService();

    // Ensure that the share is connected (idempotent)
    bool ensureConnected();

    // Save a local file as attachment for the given ticket
    // Returns info for DB (relative path, stored name, size) or nullopt on error
    std::optional<StoredFileInfo> saveAttachment(quint64 ticketId, const QString& localFilePath);

    std::optional<StoredFileInfo> saveAttachment(quint64 ticketId, const QString& localFilePath, ProgressCallback progressCallback, CancelCheckCallback cancelCheck);

    // Download attachment (by relative path from DB) to a local file
    bool loadAttachmentToFile(const QString& relativePath, const QString& localTargetPath);

    // Load attachment bytes into memory
    std::optional<QByteArray> loadAttachmentBytes(const QString& relativePath);

    // Physically delete the attachment file (DB should do soft-delete flag)
    bool deleteAttachmentFile(const QString& relativePath);

   private:
    QString buildTicketFolderName(quint64 ticketId) const;
    QString buildAbsolutePath(const QString& relativePath) const;
    QString buildTicketFolderAbsolute(quint64 ticketId) const;

    // Generates a unique filename in the given folder if file already exists
    QString generateStoredFileName(const QString& originalName, const QString& ticketFolderAbs) const;

   private:
    NetworkShareData _networkShareData;
    bool _connected{false};
};
