#include "Icons.h"
#include "Theme.h"

#include <QDir>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcIcons, "gonnect.app.icons")

Icons::Icons(QObject *parent) : QQmlPropertyMap{ this, parent }
{
    connect(&Theme::instance(), &Theme::isDarkModeChanged, this, &Icons::updateIconPaths);

    updateIconPaths();
}

void Icons::updateIconPaths()
{
    const QDir iconDir(":/icons");
    const auto fileList = iconDir.entryList({ "*.svg" }, QDir::Files | QDir::Readable);
    const bool isDarkMode = Theme::instance().isDarkMode();

    for (const auto &fileName : fileList) {

        // Extract file name and make it a property name (e.g. "media-playback-pause.svg" =>
        // "mediaPlaybackPause")
        const auto split = fileName.split('.');

        if (split.length() != 2) {
            qCCritical(lcIcons).nospace().noquote()
                    << "Cannot compute file name '" << fileName << "' - ignoring";
            continue;
        }

        auto nameParts = split.at(0).split('-');

        for (int i = 1; i < nameParts.length(); ++i) {
            const auto &part = nameParts.at(i);
            nameParts[i] = part.left(1).toUpper() + part.sliced(1);
        }
        const auto propertyName = nameParts.join("");

        // Insert into property map with qml-readable path
        QString qmlPath;
        if (isDarkMode) {
            qmlPath = QString("qrc:/icons/dark/%1").arg(fileName);
        } else {
            qmlPath = QString("qrc:/icons/%1").arg(fileName);
        }

        insert(propertyName, qmlPath);
    }
}
