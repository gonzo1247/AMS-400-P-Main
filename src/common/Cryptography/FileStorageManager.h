#pragma once

#include <QString>
#include <optional>

#include "Crypto.h"
#include "DatabaseDefines.h"

class FileStorageManager
{
   public:
    explicit FileStorageManager(QString rootPath);

    bool CreateTicketAttachment(const QString& sourceFilePath, std::uint64_t ticketID, std::uint32_t uploaderUserID, const QString& description);

    bool RestoreAttachment(std::uint64_t attachmentID, const QString& targetPath);

    std::optional<TicketAttachmentInformation> LoadAttachmentMeta(std::uint64_t id) const;

   private:
    QString buildRelativePath(const QString& originalName) const;
    QString buildStoredFilename(std::uint64_t dbId, const QString& originalName) const;

    bool computeSha256(const QString& filePath, QString& outHash) const;

    QString _rootPath;
    std::unique_ptr<Crypto> _crypto;
};
