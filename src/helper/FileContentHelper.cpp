#include "FileContentHelper.h"

#include <QMimeDatabase>
#include <QMimeType>
#include <QUrl>

FileContentHelper::FileType FileContentHelper::fileType(const QString &filePath)
{
    // Clear "file://" at beginning
    const QString cleanPath =
            QUrl(filePath).isLocalFile() ? QUrl(filePath).toLocalFile() : filePath;

    if (cleanPath.isEmpty()) {
        return FileType::Other;
    }

    const QMimeDatabase db;
    const auto mimeName = db.mimeTypeForFile(cleanPath).name();

    if (mimeName.startsWith("image/")) {
        return FileType::Image;
    }
    if (mimeName.startsWith("audio/")) {
        return FileType::Audio;
    }
    if (mimeName.startsWith("video/")) {
        return FileType::Video;
    }

    return FileType::Other;
}

FileContentHelper::FileContentHelper() { }
