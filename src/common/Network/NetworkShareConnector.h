#pragma once

#include <QByteArray>
#include <QString>
#include <optional>

class NetworkShareConnector
{
   public:
    // Connects the share using a service account
    static bool ConnectShare(const QString& remotePath, const QString& username, const QString& password);

    // Disconnects the share
    static bool DisconnectShare(const QString& remotePath);

    // Uploads a file to the network share
    static bool UploadFile(const QString& localFile, const QString& remoteFile);

    // Downloads a file from the share
    static bool DownloadFile(const QString& remoteFile, const QString& localFile);

    // Reads a file from the share into memory
    static std::optional<QByteArray> ReadFileBytes(const QString& remoteFile);

    // Writes a memory buffer to the share
    static bool WriteFileBytes(const QString& remoteFile, const QByteArray& data);
};
