#pragma once

#include <QString>

namespace MimeTypeHelper
{
    // Detect MIME type by content and extension
    QString detectMimeType(const QString& filePath);

    // Detect MIME type only by extension (fallback)
    QString mimeFromExtension(const QString& filePath);
}  // namespace MimeTypeHelper
