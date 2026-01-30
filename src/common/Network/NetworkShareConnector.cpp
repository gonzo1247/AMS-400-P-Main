#include "pch.h"

#include "NetworkShareConnector.h"

#include <Windows.h>
#include <Winnetwk.h>

#include <QDir>
#include <QFile>

#include "Logger.h"
#include "LoggerDefines.h"

// Helper macro for wide strings
static std::wstring ToW(const QString& s)
{
    return std::wstring(reinterpret_cast<const wchar_t*>(s.utf16()));
}

bool NetworkShareConnector::ConnectShare(const QString& remotePath, const QString& username, const QString& password)
{
    QString remotePathTrimmed = remotePath.trimmed();

    // Ensure UNC prefix
    if (!remotePathTrimmed.startsWith("\\\\"))
        remotePathTrimmed.prepend("\\\\");

    std::wstring remote = remotePathTrimmed.toStdWString();
    std::wstring user = username.trimmed().toStdWString();
    std::wstring pass = password.toStdWString();

    NETRESOURCEW nr;
    ZeroMemory(&nr, sizeof(nr));

    nr.dwType = RESOURCETYPE_DISK;
    nr.dwDisplayType = RESOURCEDISPLAYTYPE_SHARE;
    nr.dwUsage = RESOURCEUSAGE_CONNECTABLE;
    nr.lpLocalName = nullptr;
    nr.lpRemoteName = const_cast<LPWSTR>(remote.c_str());
    nr.lpProvider = const_cast<LPWSTR>(L"Microsoft Windows Network");

    LOG_DEBUG("ConnectShare: user: {}, path: {}, password: {}", QString::fromStdWString(user).toStdString(),
              QString::fromStdWString(remote).toStdString(), QString::fromStdWString(pass).toStdString());

    DWORD result = WNetAddConnection2W(&nr,
                                       pass.empty() ? nullptr : pass.c_str(),
                                       user.empty() ? nullptr : user.c_str(),
                                       CONNECT_TEMPORARY);

    if (result == NO_ERROR || result == ERROR_ALREADY_ASSIGNED)
        return true;
            
    LPWSTR msgBuf = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;

    DWORD size = FormatMessageW(flags, nullptr, result, 0, reinterpret_cast<LPWSTR>(&msgBuf), 0, nullptr);

    QString msg = size && msgBuf ? QString::fromWCharArray(msgBuf).trimmed() : QStringLiteral("Unknown error");

    if (msgBuf)
        LocalFree(msgBuf);

    LOG_DEBUG("ConnectShare failed: code = {} message = {}", result, msg.toStdString());

    return false;
}

bool NetworkShareConnector::DisconnectShare(const QString& remotePath)
{
    DWORD result = WNetCancelConnection2W(ToW(remotePath).c_str(), 0, TRUE);

    return (result == NO_ERROR);
}

bool NetworkShareConnector::UploadFile(const QString& localFile, const QString& remoteFile)
{
    QFile src(localFile);
    if (!src.open(QIODevice::ReadOnly))
    {
        LOG_DEBUG("UploadFile: failed to open source file {} error: {}", localFile, src.errorString());
        return false;
    }

    QFileInfo fi(remoteFile);
    QDir dir = fi.dir();
    if (!dir.exists())
    {
        // Try to create target directory
        if (!dir.mkpath("."))
        {
            LOG_DEBUG("UploadFile: failed to create target directory {}", dir.absolutePath());
            return false;
        }
    }

    QFile dst(remoteFile);
    if (!dst.open(QIODevice::WriteOnly))
    {
        LOG_DEBUG("UploadFile: failed to open destination file {} error: {}", remoteFile, dst.errorString());
        return false;
    }

    const qint64 written = dst.write(src.readAll());
    if (written == -1)
    {
        LOG_DEBUG("UploadFile: write failed for {} error: {}", remoteFile, dst.errorString());
        return false;
    }

    return true;
}

bool NetworkShareConnector::DownloadFile(const QString& remoteFile, const QString& localFile)
{
    QFile src(remoteFile);
    if (!src.exists() || !src.open(QIODevice::ReadOnly))
        return false;

    QFile dst(localFile);
    if (!dst.open(QIODevice::WriteOnly))
        return false;

    dst.write(src.readAll());
    return true;
}

std::optional<QByteArray> NetworkShareConnector::ReadFileBytes(const QString& remoteFile)
{
    QFile f(remoteFile);
    if (!f.open(QIODevice::ReadOnly))
        return std::nullopt;

    return f.readAll();
}

bool NetworkShareConnector::WriteFileBytes(const QString& remoteFile, const QByteArray& data)
{
    QFile f(remoteFile);
    if (!f.open(QIODevice::WriteOnly))
        return false;

    f.write(data);
    return true;
}
