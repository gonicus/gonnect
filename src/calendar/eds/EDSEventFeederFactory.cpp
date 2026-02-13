#include "EDSEventFeeder.h" // INFO: Import prior to any Qt imports!
#include "EDSEventFeederFactory.h"
#include "ReadOnlyConfdSettings.h"
#include "DateEventFeederManager.h"

EDSEventFeederFactory::EDSEventFeederFactory(QObject *parent) : QObject{ parent } { }

QStringList EDSEventFeederFactory::configurations() const
{
    const QString groupName = "eds-calendar";

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

IDateEventFeeder *EDSEventFeederFactory::createFeeder(const QString &settingsGroup,
                                                      const QDateTime &currentTime,
                                                      const QDateTime &timeRangeStart,
                                                      const QDateTime &timeRangeEnd,
                                                      DateEventFeederManager *feederManager) const
{
    ReadOnlyConfdSettings settings;
    settings.beginGroup(settingsGroup);

    return new EDSEventFeeder(feederManager, name(), currentTime, timeRangeStart, timeRangeEnd);
}
