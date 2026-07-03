#pragma once

#include <QObject>
#include <QQmlEngine>

class FileHelper : public QObject
{
    Q_OBJECT

public:
    static FileHelper &instance()
    {
        static FileHelper fh;
        return fh;
    };

    /// List of file name filters for supported image files (as used by a file picker dialog)
    Q_INVOKABLE QStringList imageFileSelectors() const;

    /// List of file name filters for supported audio and video files (as used by a file picker
    /// dialog)
    Q_INVOKABLE QStringList mediaFileSelectors(const bool includeVideo = true) const;

    /// Copy file from fromPath to toPath. Path can begin with "file://" or not.
    Q_INVOKABLE bool copyFile(const QString &fromPath, const QString &toPath) const;

    /// Return just the file name from a complete path.
    Q_INVOKABLE QString fileNameFromPath(const QString &path) const;

    /// Return absolute path to the default download folder.
    Q_INVOKABLE QString downloadFolderPath() const;

    /// Creates a file path to a log file from the name and ensures the folder path exists.
    QString makeLogFilePath(const QString &name) const;

private:
    explicit FileHelper(QObject *parent = nullptr);
};

class FileHelperWrapper
{
    Q_GADGET
    QML_FOREIGN(FileHelper)
    QML_NAMED_ELEMENT(FileHelper)
    QML_SINGLETON

public:
    static FileHelper *create(QQmlEngine *, QJSEngine *)
    {
        QQmlEngine::setObjectOwnership(&FileHelper::instance(), QQmlEngine::CppOwnership);
        return &FileHelper::instance();
    }

private:
    FileHelperWrapper() = default;
};
