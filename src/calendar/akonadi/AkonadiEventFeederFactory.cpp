#include "AkonadiEventFeederFactory.h"
#include "ReadOnlyConfdSettings.h"
#include "DateEventFeederManager.h"
#include "AkonadiEventFeeder.h"

AkonadiEventFeederFactory::AkonadiEventFeederFactory(QObject *parent) : QObject{ parent } { }

QStringList AkonadiEventFeederFactory::configurations() const
{
    const QString groupName = "akonadi-calendar";

    ReadOnlyConfdSettings settings;
    const QStringList groups = settings.childGroups();

    if (groups.contains(groupName)) {
        settings.beginGroup(groupName);
        const bool enabled = settings.value("enabled", false).toBool();
        settings.endGroup();

        if (!enabled) {
            return {};
        }
    }

    return { groupName };
}

IDateEventFeeder *AkonadiEventFeederFactory::createFeeder(
        const QString &settingsGroup, const QDateTime &currentTime,
        const QDateTime &timeRangeStart, const QDateTime &timeRangeEnd,
        DateEventFeederManager *feederManager) const
{
    ReadOnlyConfdSettings settings;
    settings.beginGroup(settingsGroup);

    return new AkonadiEventFeeder(feederManager, name(), currentTime, timeRangeStart, timeRangeEnd);
}
