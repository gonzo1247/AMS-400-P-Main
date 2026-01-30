#include "pch.h"
#include "FileStorageManager.h"

#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

#include "ConnectionGuard.h"
#include "Crypto.h"
#include "FileCrypto.h"
#include "Logger.h"
#include "MimeTypeHelper.h"
#include "Util.h"

FileStorageManager::FileStorageManager(QString rootPath) : _rootPath(std::move(rootPath))
{
    _crypto = std::make_unique<Crypto>();
}

// folder: attachments/YYYY/MM/DD/
QString FileStorageManager::buildRelativePath(const QString& originalName) const
{
    const QDate d = QDate::currentDate();
    return QString("attachments/%1/%2/%3/")
        .arg(d.year())
        .arg(d.month(), 2, 10, QChar('0'))
        .arg(d.day(), 2, 10, QChar('0'));
}

// stored file name: 0000001234_filename.ext.enc
QString FileStorageManager::buildStoredFilename(std::uint64_t dbId, const QString& originalName) const
{
    QString safe = originalName;
    safe.replace('/', '_');
    safe.replace('\\', '_');
    return QString("%1_%2.enc").arg(QString::number(dbId).rightJustified(10, '0')).arg(safe);
}

bool FileStorageManager::computeSha256(const QString& filePath, QString& outHash) const
{
    try
    {
        const std::string hash = _crypto->Sha256File(filePath.toStdString());
        outHash = QString::fromStdString(hash);
        return true;
    }
    catch (...)
    {
        LOG_ERROR("FileStorageManager::computeSha256: failed hashing {}", filePath.toStdString());
        return false;
    }
}

bool FileStorageManager::CreateTicketAttachment(const QString& sourceFilePath, std::uint64_t ticketID, std::uint32_t uploaderUserID, const QString& description)
{
    QFileInfo info(sourceFilePath);
    if (!info.exists() || !info.isFile())
    {
        LOG_ERROR("CreateTicketAttachment: source not found {}", sourceFilePath.toStdString());
        return false;
    }

    const QString originalFilename = info.fileName();
    const std::uint64_t originalSize = info.size();

    // --- Compute original hash ---
    QString shaOriginal;
    if (!computeSha256(sourceFilePath, shaOriginal))
        return false;

    // --- Read file ---
    QFile in(sourceFilePath);
    if (!in.open(QIODevice::ReadOnly))
        return false;

    QByteArray plainData = in.readAll();
    in.close();

    std::string plainStr(plainData.begin(), plainData.end());

    // --- Encrypt ---
    auto encryptedOpt = FileCrypto::Encrypt(plainStr);
    if (!encryptedOpt.has_value())
        return false;

    const auto& blob = *encryptedOpt;
    const std::uint64_t encryptedSize = blob.size();

    // --- Insert DB row (ID auto assigned) ---
    std::uint64_t newID = 0;
    {
        ConnectionGuardAMS conn(ConnectionType::Sync);
        auto stmt = conn->GetPreparedStatement(AMSPreparedStatement::DB_TATT_INSERT_EMPTY);

        stmt->SetUInt64(0, ticketID);
        stmt->SetUInt(1, uploaderUserID);
        stmt->SetString(2, Util::CurrentDateTimeStringStd());  // uploadedAt
        stmt->SetString(3, originalFilename.toStdString());
        stmt->SetString(4, description.toStdString());
        stmt->SetUInt64(5, originalSize);
        stmt->SetString(6, shaOriginal.toStdString());

        conn->ExecutePreparedInsert(*stmt);
        newID = conn->GetLastInsertId();
    }

    if (newID == 0)
        return false;

    // --- Build paths ---
    const QString relPath = buildRelativePath(originalFilename);
    const QString storedFileName = buildStoredFilename(newID, originalFilename);
    const QString fullFolderPath = QDir(_rootPath).filePath(relPath);

    QDir dir(fullFolderPath);
    if (!dir.exists() && !dir.mkpath("."))
        return false;

    const QString fullFilePath = dir.filePath(storedFileName);

    // --- Save encrypted blob ---
    QSaveFile out(fullFilePath);
    if (!out.open(QIODevice::WriteOnly))
        return false;

    out.write(reinterpret_cast<const char*>(blob.data()), blob.size());
    if (!out.commit())
        return false;

    // --- Compute encrypted hash ---
    QString shaEncrypted;
    if (!computeSha256(fullFilePath, shaEncrypted))
        return false;

    // --- Update DB with final path + encrypted info ---
    {
        const QString mime = MimeTypeHelper::detectMimeType(originalFilename);
        ConnectionGuardAMS conn(ConnectionType::Sync);
        auto stmt = conn->GetPreparedStatement(AMSPreparedStatement::DB_TATT_UPDATE_META);

        stmt->SetString(0, storedFileName.toStdString());
        stmt->SetString(1, relPath.toStdString());
        stmt->SetUInt64(2, encryptedSize);
        stmt->SetString(3, shaEncrypted.toStdString());
        stmt->SetString(4, mime.toStdString());
        stmt->SetUInt64(5, newID);

        conn->ExecutePreparedUpdate(*stmt);
    }

    return true;
}

std::optional<TicketAttachmentInformation> FileStorageManager::LoadAttachmentMeta(std::uint64_t id) const
{
    try
    {
        ConnectionGuardAMS conn(ConnectionType::Sync);
        auto stmt = conn->GetPreparedStatement(AMSPreparedStatement::DB_TATT_SELECT_BY_ID);
        stmt->SetUInt64(0, id);

        auto result = conn->ExecutePreparedSelect(*stmt);
        if (!result.IsValid())
            return std::nullopt;

        auto* row = result.Fetch();
        if (!row)
            return std::nullopt;

        TicketAttachmentInformation info;
        info.id = row[0].GetUInt64();
        info.ticketID = row[1].GetUInt64();
        info.uploaderUserID = row[2].GetUInt32();
        info.uploadedAt = row[3].GetDateTime();
        info.originalFilename = row[4].GetString();
        info.storedFileName = row[5].GetString();
        info.filePath = row[6].GetString();
        info.mimeType = row[7].GetString();
        info.fileSize = row[8].GetUInt64();
        info.description = row[9].GetString();
        info.encryptedSize = row[10].GetUInt64();
        info.sha256Original = row[11].GetString();
        info.sha256Encrypted = row[12].GetString();
        info.isDeleted = row[13].GetBool();

        return info;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

bool FileStorageManager::RestoreAttachment(std::uint64_t attachmentID, const QString& targetPath)
{
    auto metaOpt = LoadAttachmentMeta(attachmentID);
    if (!metaOpt)
        return false;

    const auto& meta = *metaOpt;

    const QString fullPath =
        QDir(_rootPath).filePath(QString::fromStdString(meta.filePath) + QString::fromStdString(meta.storedFileName));

    // Hash check first
    QString shaNow;
    if (!computeSha256(fullPath, shaNow))
        return false;

    if (shaNow.toStdString() != meta.sha256Encrypted)
    {
        LOG_ERROR("RestoreAttachment: encrypted hash mismatch");
        return false;
    }

    QFile in(fullPath);
    if (!in.open(QIODevice::ReadOnly))
        return false;

    QByteArray blob = in.readAll();
    in.close();

    std::vector<std::uint8_t> blobVec(blob.begin(), blob.end());

    auto plainOpt = FileCrypto::Decrypt(blobVec);
    if (!plainOpt)
        return false;

    const std::string& plain = *plainOpt;

    QFile out(targetPath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    out.write(plain.data(), plain.size());
    out.close();

    return true;
}
