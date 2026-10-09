#pragma once

#include <QObject>
#include <QQmlEngine>
#include "ChatMessageContentFile.h"

class ChatMessageContentVideoFile : public ChatMessageContentFile
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(
            QString thumbnailFilePath READ thumbnailFilePath NOTIFY thumbnailFilePathChanged FINAL)

public:
    explicit ChatMessageContentVideoFile(const QString &videoFilePath,
                                         const QString &videoFileName = "",
                                         QObject *parent = nullptr);

    QString thumbnailFilePath() const { return m_thumbnailFilePath; }

private:
    static QString ensuredThumbnailDirPath();

    void setThumbnailFilePath(const QString &filePath);

    QString m_thumbnailFilePath;

private Q_SLOTS:
    void updateThumbnail();

Q_SIGNALS:
    void thumbnailFilePathChanged();
};
