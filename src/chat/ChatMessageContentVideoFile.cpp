#include "ChatMessageContentVideoFile.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QVideoFrame>
#include <QStandardPaths>

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcChatMessageContentVideoFile, "gonnect.app.chat.message.content.video")

ChatMessageContentVideoFile::ChatMessageContentVideoFile(const QString &videoFilePath,
                                                         const QString &videoFileName,
                                                         QObject *parent)
    : ChatMessageContentFile{ videoFilePath, videoFileName, parent }
{
    connect(this, &ChatMessageContentVideoFile::filePathChanged, this,
            &ChatMessageContentVideoFile::updateThumbnail);

    if (!videoFilePath.isEmpty()) {
        updateThumbnail();
    }
}

QString ChatMessageContentVideoFile::ensuredThumbnailDirPath()
{
    static const QString thumbnailDirPath =
            QString("%1/thumbnails")
                    .arg(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));

    static bool _isInitialized = false;

    if (!_isInitialized) {
        _isInitialized = true;

        if (!QDir().mkpath(thumbnailDirPath)) {
            qCCritical(lcChatMessageContentVideoFile)
                    << "Unable to create directory" << thumbnailDirPath;
        }
    }

    return thumbnailDirPath;
}

void ChatMessageContentVideoFile::setThumbnailFilePath(const QString &filePath)
{
    if (m_thumbnailFilePath != filePath) {
        m_thumbnailFilePath = filePath;
        Q_EMIT thumbnailFilePathChanged();
    }
}

void ChatMessageContentVideoFile::updateThumbnail()
{
    static const QString thumbnailDirPath = ensuredThumbnailDirPath();

    // Clear image file path
    auto path = filePath();
    static const QString fileProtocol("file://");
    if (path.startsWith(fileProtocol)) {
        path.remove(0, fileProtocol.length());
    }

    // Check path
    if (path.isEmpty()) {
        setThumbnailFilePath("");
        return;
    }

    QFile imgFile(path);
    if (!imgFile.exists()) {
        qCCritical(lcChatMessageContentVideoFile)
                << "Image file" << path << "is supposed to exist but does not";
        setThumbnailFilePath("");
        return;
    }

    // Create hash for image file
    if (!imgFile.open(QIODevice::ReadOnly)) {
        qCCritical(lcChatMessageContentVideoFile) << "Unable to open image file" << path;
        setThumbnailFilePath("");
        return;
    }

    QCryptographicHash fileHash(QCryptographicHash::Sha256);
    QByteArray hash;
    if (fileHash.addData(&imgFile)) {
        hash = fileHash.result().toHex();
    } else {
        qCCritical(lcChatMessageContentVideoFile) << "Unable to calculate hash for" << path;
        setThumbnailFilePath("");
        return;
    }

    // Check if thumbnail already exists
    const auto thumbnailPath = QString("%1/%2.png").arg(thumbnailDirPath).arg(hash);

    if (QFileInfo::exists(thumbnailPath)) {
        setThumbnailFilePath(QString("file://%1").arg(thumbnailPath));
    } else {
        auto *player = new QMediaPlayer(this);
        auto *sink = new QVideoSink(player);
        player->setVideoSink(sink);
        player->setSource(path);

        connect(
                sink, &QVideoSink::videoFrameChanged, this,
                [this, thumbnailPath, player](const QVideoFrame &frame) {
                    if (frame.isValid()) {
                        QImage img = frame.toImage();
                        img.save(thumbnailPath);

                        player->stop();
                        player->deleteLater();

                        setThumbnailFilePath(QString("file://%1").arg(thumbnailPath));
                    }
                },
                Qt::SingleShotConnection);

        player->play();
    }
}
