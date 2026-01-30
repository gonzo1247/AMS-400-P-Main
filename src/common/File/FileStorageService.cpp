#include "pch.h"

#include "FileStorageService.h"

#include <QUuid>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRandomGenerator>

#include "NetworkShareConnector.h"

FileStorageService::FileStorageService()
{
    _networkShareData = GetSettings().getNetworkShareData();
}

bool FileStorageService::ensureConnected()
{
    if (!_networkShareData.networkShareActive)
        return false;

    if (_connected)
        return true;

    if (_networkShareData.networkSharePath.isEmpty())
        return false;

    const QString& shareRoot = _networkShareData.networkSharePath;

    // Connect network share via service account
    if (!NetworkShareConnector::ConnectShare(shareRoot, _networkShareData.networkShareUser, _networkShareData.networkSharePassword))
        return false;

    _connected = true;
    return true;
}

QString FileStorageService::buildTicketFolderName(quint64 ticketId) const
{
    // Example: "TICKET_0000006"
    return QStringLiteral("TICKET_%1").arg(ticketId, 7, 10, QChar('0'));
}

QString FileStorageService::buildTicketFolderAbsolute(quint64 ticketId) const
{
    QString abs = _networkShareData.networkSharePath;
    if (!abs.endsWith('\\') && !abs.endsWith('/'))
        abs += '\\';

    abs += buildTicketFolderName(ticketId);
    return abs;
}

QString FileStorageService::buildAbsolutePath(const QString& relativePath) const
{
    if (relativePath.isEmpty())
        return {};

    QString abs = _networkShareData.networkSharePath;
    if (!abs.endsWith('\\') && !abs.endsWith('/'))
        abs += '\\';

    abs += relativePath;
    return abs;
}

QString FileStorageService::generateStoredFileName(const QString& originalName, const QString& ticketFolderAbs) const
{
    QFileInfo fi(originalName);
    const QString ext = fi.suffix().toLower();  // extension without dot

    QDir dir(ticketFolderAbs);

    // Try multiple times in case a UUID already exists (extremely unlikely)
    for (int i = 0; i < 5; ++i)
    {
        // Create UUID without braces: 550e8400-e29b-41d4-a716-446655440000
        QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);

        QString candidate = uuid;
        if (!ext.isEmpty())
            candidate += "." + ext;

        // Absolute path for checking collisions
        const QString fullPath = dir.filePath(candidate);

        if (!QFileInfo::exists(fullPath))
            return candidate;
    }

    // Fallback: use a timestamp + random number
    QString fallback = QStringLiteral("%1_%2").arg(QDateTime::currentSecsSinceEpoch()).arg(QRandomGenerator::global()->generate());

    if (!ext.isEmpty())
        fallback += "." + ext;

    return fallback;
}

std::optional<FileStorageService::StoredFileInfo> FileStorageService::saveAttachment(quint64 ticketId, const QString& localFilePath)
{
    return saveAttachment(ticketId, localFilePath, {}, []() { return false; });
}

std::optional<FileStorageService::StoredFileInfo> FileStorageService::saveAttachment(quint64 ticketId, const QString& localFilePath, ProgressCallback progressCallback, CancelCheckCallback cancelCheck)
{
    if (!ensureConnected())
        return std::nullopt;

    QFileInfo srcInfo(localFilePath);
    if (!srcInfo.exists() || !srcInfo.isFile())
    {
        LOG_ERROR("saveAttachment: Source file does not exist: {}", localFilePath.toStdString());
        return std::nullopt;
    }

    QFile src(localFilePath);
    if (!src.open(QIODevice::ReadOnly))
    {
        LOG_ERROR("saveAttachment: Failed to open source file: {}", localFilePath.toStdString());
        return std::nullopt;
    }

    const quint64 totalBytes = static_cast<quint64>(srcInfo.size());

    const QString originalName = srcInfo.fileName();
    const QString ticketFolderAbs = buildTicketFolderAbsolute(ticketId);
    QDir ticketDir(ticketFolderAbs);

    if (!ticketDir.exists())
    {
        if (!ticketDir.mkpath(QStringLiteral(".")))
        {
            LOG_ERROR("saveAttachment: Failed to create ticket directory: {}", ticketFolderAbs.toStdString());
            return std::nullopt;
        }
    }

    const QString storedName = generateStoredFileName(originalName, ticketFolderAbs);

    QString targetAbsPath = ticketFolderAbs;
    if (!targetAbsPath.endsWith('\\') && !targetAbsPath.endsWith('/'))
        targetAbsPath += '\\';
    targetAbsPath += storedName;

    QFile dst(targetAbsPath);
    if (!dst.open(QIODevice::WriteOnly))
    {
        LOG_ERROR("saveAttachment: Failed to open destination file: {}", targetAbsPath.toStdString());
        return std::nullopt;
    }

    const qint64 chunkSize = 1024 * 1024;  // 1 MB
    qint64 writtenTotal = 0;

    while (true)
    {
        if (cancelCheck && cancelCheck())
            return std::nullopt;

        QByteArray chunk = src.read(chunkSize);
        if (chunk.isEmpty())
        {
            if (src.atEnd())
                break;

            LOG_ERROR("saveAttachment: Read error on source file: {}", localFilePath.toStdString());
            return std::nullopt;
        }

        const qint64 written = dst.write(chunk);
        if (written != chunk.size())
        {
            LOG_ERROR("saveAttachment: Failed to write full chunk to: {}", targetAbsPath.toStdString());
            return std::nullopt;
        }

        writtenTotal += written;

        if (progressCallback)
            progressCallback(writtenTotal, totalBytes);
    }

    dst.flush();
    dst.close();
    src.close();

    StoredFileInfo info;
    info.storedFileName = storedName;
    info.relativePath = buildTicketFolderName(ticketId) + "/" + storedName;
    info.fileSize = static_cast<quint64>(writtenTotal);

    LOG_DEBUG("saveAttachment: Stored '{}' as '{}' (relative '{}')", localFilePath.toStdString(),
              storedName.toStdString(), info.relativePath.toStdString());

    return info;
}

bool FileStorageService::loadAttachmentToFile(const QString& relativePath, const QString& localTargetPath)
{
    if (!ensureConnected())
        return false;

    const QString abs = buildAbsolutePath(relativePath);
    if (abs.isEmpty())
        return false;

    // Ensure local directory exists
    QFileInfo fi(localTargetPath);
    QDir localDir = fi.dir();
    if (!localDir.exists())
    {
        if (!localDir.mkpath(QStringLiteral(".")))
            return false;
    }

    QFile src(abs);
    if (!src.open(QIODevice::ReadOnly))
        return false;

    QFile dst(localTargetPath);
    if (!dst.open(QIODevice::WriteOnly))
        return false;

    dst.write(src.readAll());
    return true;
}

std::optional<QByteArray> FileStorageService::loadAttachmentBytes(const QString& relativePath)
{
    if (!ensureConnected())
        return std::nullopt;

    const QString abs = buildAbsolutePath(relativePath);
    if (abs.isEmpty())
        return std::nullopt;

    QFile f(abs);
    if (!f.open(QIODevice::ReadOnly))
        return std::nullopt;

    return f.readAll();
}

bool FileStorageService::deleteAttachmentFile(const QString& relativePath)
{
    if (!ensureConnected())
        return false;

    const QString abs = buildAbsolutePath(relativePath);
    if (abs.isEmpty())
        return false;

    QFile f(abs);
    if (!f.exists())
        return true;  // already gone

    return f.remove();
}
