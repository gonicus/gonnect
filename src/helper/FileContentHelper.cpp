#include "FileContentHelper.h"

#include <QMimeDatabase>
#include <QMimeType>
#include <QFileInfo>

FileContentHelper::FileContentHelper(QObject *parent) : QObject{ parent } { }

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

bool FileContentHelper::isLocalFileAndNotDirectory(const QUrl &url) const
{
    if (!url.isLocalFile()) {
        return false;
    }
    const QFileInfo checkFile(url.toLocalFile());
    return checkFile.isFile();
}
