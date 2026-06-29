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

    Q_INVOKABLE bool isLocalFileAndNotDirectory(const QUrl &url) const;

private:
    FileContentHelper(QObject *parent = nullptr);
};
