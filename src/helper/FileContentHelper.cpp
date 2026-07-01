#include "FileContentHelper.h"

#include <QMimeDatabase>
#include <QMimeType>
#include <QFileInfo>
#include <QDirIterator>
#include <QDir>

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

bool FileContentHelper::isLocalFile(const QUrl &url) const
{
    if (!url.isLocalFile()) {
        return false;
    }
    const QFileInfo checkFile(url.toLocalFile());
    return checkFile.isFile();
}

bool FileContentHelper::isLocalDirectory(const QUrl &url) const
{
    if (!url.isLocalFile()) {
        return false;
    }
    const QFileInfo checkFile(url.toLocalFile());
    return checkFile.isDir();
}

bool FileContentHelper::isLocalReadable(const QUrl &url) const
{
    if (!url.isLocalFile()) {
        return false;
    }
    const QFileInfo checkFile(url.toLocalFile());
    return checkFile.isReadable();
}

QList<QUrl> FileContentHelper::uploadableUrls(const QList<QUrl> &urls) const
{
    QList<QUrl> result;
    result.reserve(urls.size());

    const auto processFile = [&result, this](const QUrl &url) {
        if (isLocalReadable(url)) {
            result.append(url);
        }
    };

    for (const auto &url : urls) {
        if (!isLocalReadable(url)) {
            continue;
        }

        if (isLocalFile(url)) {
            processFile(url);
        } else if (isLocalDirectory(url)) {
            QDirIterator it(url.toLocalFile(), QDir::Files | QDir::NoDotAndDotDot,
                            QDirIterator::Subdirectories);

            while (it.hasNext()) {
                it.next();
                const QFileInfo info = it.fileInfo();

                if (info.isFile() && !info.isSymLink()) {
                    processFile(QUrl::fromLocalFile(info.absoluteFilePath()));
                }
            }
        }
    }

    return result;
}
