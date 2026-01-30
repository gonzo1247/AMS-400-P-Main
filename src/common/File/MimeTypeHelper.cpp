#include "pch.h"

#include "MimeTypeHelper.h"

#include <QFileInfo>
#include <QMimeDatabase>

namespace MimeTypeHelper
{

    QString detectMimeType(const QString& filePath)
    {
        QMimeDatabase db;

        // Try by content first
        QMimeType mt = db.mimeTypeForFile(filePath, QMimeDatabase::MatchContent);

        if (mt.isValid() && !mt.isDefault())
            return mt.name();

        // Fallback: try by extension via QMimeDatabase
        mt = db.mimeTypeForFile(filePath, QMimeDatabase::MatchExtension);

        if (mt.isValid() && !mt.isDefault())
            return mt.name();

        // Last fallback: custom mapping
        return mimeFromExtension(filePath);
    }

    QString mimeFromExtension(const QString& filePath)
    {
        QFileInfo fi(filePath);
        const QString ext = fi.suffix().toLower();

        if (ext == "pdf")
            return QStringLiteral("application/pdf");
        if (ext == "png")
            return QStringLiteral("image/png");
        if (ext == "jpg" || ext == "jpeg")
            return QStringLiteral("image/jpeg");
        if (ext == "gif")
            return QStringLiteral("image/gif");
        if (ext == "bmp")
            return QStringLiteral("image/bmp");
        if (ext == "txt")
            return QStringLiteral("text/plain");
        if (ext == "log")
            return QStringLiteral("text/plain");
        if (ext == "csv")
            return QStringLiteral("text/csv");
        if (ext == "xml")
            return QStringLiteral("application/xml");
        if (ext == "json")
            return QStringLiteral("application/json");
        if (ext == "zip")
            return QStringLiteral("application/zip");
        if (ext == "7z")
            return QStringLiteral("application/x-7z-compressed");
        if (ext == "rar")
            return QStringLiteral("application/vnd.rar");
        if (ext == "doc")
            return QStringLiteral("application/msword");
        if (ext == "docx")
            return QStringLiteral("application/vnd.openxmlformats-officedocument.wordprocessingml.document");
        if (ext == "xls")
            return QStringLiteral("application/vnd.ms-excel");
        if (ext == "xlsx")
            return QStringLiteral("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet");

        // Fallback
        return QStringLiteral("application/octet-stream");
    }

}  // namespace MimeTypeHelper
