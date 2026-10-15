#pragma once

#include <qobjectdefs.h>
#include <qqmlintegration.h>
#include <QString>
#include <QUrl>
#include <QQmlEngine>

class FileContentHelper : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString portalFileTransferMimeType READ portalFileTransferMimeType CONSTANT)

public:
    enum class FileType { Image, Audio, Video, Other };
    Q_ENUM(FileType)

    static FileContentHelper &instance()
    {
        static FileContentHelper h;
        return h;
    }
    static FileContentHelper *create(QQmlEngine *, QJSEngine *)
    {
        QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
        return &instance();
    }

    static FileContentHelper::FileType fileType(const QString &filePath);

    Q_INVOKABLE bool isLocalFile(const QUrl &url) const;
    Q_INVOKABLE bool isLocalDirectory(const QUrl &url) const;
    Q_INVOKABLE bool isLocalReadable(const QUrl &url) const;

    Q_INVOKABLE QList<QUrl> uploadableUrls(const QList<QUrl> &urls) const;
    Q_INVOKABLE QList<QUrl> resolvePortalFileToken(const QString &token) const;

private:
    FileContentHelper(QObject *parent = nullptr);

    QString portalFileTransferMimeType() const;
};
