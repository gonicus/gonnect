#include "FileContentHelper.h"

#include <QMimeDatabase>
#include <QMimeType>
#include <QFileInfo>
#include <QDirIterator>
#include <QDir>
#include <QLoggingCategory>

#ifdef Q_OS_FLATPAK
#  include <QDBusConnection>
#  include <QDBusMessage>
#  include <QDBusReply>
#endif

Q_LOGGING_CATEGORY(lcFileContentHelper, "gonnect.helper.FileContentHelper")

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

QString FileContentHelper::portalFileTransferMimeType() const
{
    static const QString portalMimeType = QStringLiteral("application/vnd.portal.filetransfer");
    return portalMimeType;
}

QList<QUrl> FileContentHelper::resolvePortalFileToken(const QString &token) const
{
#ifdef Q_OS_FLATPAK
    const QString key = token.trimmed();

    if (key.isEmpty()) {
        return {};
    }

    auto message = QDBusMessage::createMethodCall(
            QStringLiteral("org.freedesktop.portal.Documents"),
            QStringLiteral("/org/freedesktop/portal/documents"),
            QStringLiteral("org.freedesktop.portal.FileTransfer"), QStringLiteral("RetrieveFiles"));
    message.setArguments({ key, QVariantMap{} });

    const QDBusReply<QStringList> reply =
            QDBusConnection::sessionBus().call(message, QDBus::Block, 5000);

    if (!reply.isValid()) {
        qCWarning(lcFileContentHelper) << "Could not retrieve files for portal file transfer token:"
                                       << reply.error().name() << reply.error().message();
        return {};
    }

    const QStringList paths = reply.value();
    QList<QUrl> urls;
    urls.reserve(paths.size());

    for (const QString &path : std::as_const(paths)) {
        urls.append(QUrl::fromLocalFile(path));
    }

    qCDebug(lcFileContentHelper) << "Portal file transfer resolved to" << urls.size() << "file(s)";

    return urls;
#else
    Q_UNUSED(token)
    return {};
#endif
}
