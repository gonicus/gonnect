#include "FileHelper.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QMediaFormat>
#include <QMimeType>
#include <QImageReader>
#include <QStandardPaths>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcFileHelper, "gonnect.app.ui.FileHelper")

FileHelper::FileHelper(QObject *parent) : QObject{ parent } { }

QStringList FileHelper::imageFileSelectors() const
{
    const auto formats = QImageReader::supportedImageFormats();
    QStringList suffixes(formats.size());
    std::transform(formats.constBegin(), formats.constEnd(), suffixes.begin(),
                   [](const QByteArray &s) { return QString("*.%1").arg(s); });

    return { tr("Image Files (%1)").arg(suffixes.join(QChar(' '))) };
}

QStringList FileHelper::mediaFileSelectors(const bool includeVideo) const
{
    QStringList result;
    QStringList allSuffixes;
    const auto formats = QMediaFormat().supportedFileFormats(QMediaFormat::Decode);

    for (const auto format : formats) {
        QMediaFormat mediaFormat(format);

        const bool isVideoAvailable =
                !mediaFormat.supportedVideoCodecs(QMediaFormat::Decode).isEmpty();

        if (!includeVideo && isVideoAvailable) {
            continue;
        }

        const auto mimeType = mediaFormat.mimeType();
        if (mimeType.isValid()) {
            const QString filter = QMediaFormat::fileFormatDescription(format);
            const auto suffixes = mimeType.suffixes();

            QStringList globs;
            globs.reserve(suffixes.length());

            for (const auto &suffix : suffixes) {
                globs.append(QString("*.%1").arg(suffix));
            }
            allSuffixes.append(globs);
            result.append(QString("%1 (%2)").arg(filter, globs.join(QChar(' '))));
        }
    }

    std::sort(result.begin(), result.end());

    QString label = tr("Audio Files");
    if (includeVideo) {
        label = tr("Media Files");
    }

    result.push_front(tr("%1 (%2)").arg(label, allSuffixes.join(QChar(' '))));

    return result;
}

bool FileHelper::copyFile(const QString &fromPath, const QString &toPath) const
{
    const auto clean = [](const QString &path) -> QString {
        return path.startsWith("file://") ? path.mid(7) : path;
    };

    const auto from = clean(fromPath);
    const auto to = clean(toPath);

    QFile toFile(to);
    if (toFile.exists()) {
        toFile.remove();
    }

    return QFile::copy(from, to);
}

QString FileHelper::fileNameFromPath(const QString &path) const
{
    return QFile(path).fileName();
}

QString FileHelper::downloadFolderPath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
}

QString FileHelper::makeLogFilePath(const QString &name) const
{
    const QString baseDirStr =
            QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    const QString subFolderName = "log";

    QDir baseDir(baseDirStr);
    if (!baseDir.mkpath(subFolderName)) {
        qCCritical(lcFileHelper) << "Error, folder" << baseDir.filePath(subFolderName)
                                 << "could not be created";
        return QString();
    }

    baseDir.cd(subFolderName);
    const QString finalFilePath = baseDir.filePath(name + QStringLiteral(".log"));

    qCInfo(lcFileHelper) << "Created log file path" << finalFilePath;
    return finalFilePath;
}
