#include "CalDAVEventFeederFactory.h"
#include "ReadOnlyConfdSettings.h"
#include "DateEventFeederManager.h"
#include "CalDAVEventFeeder.h"

CalDAVEventFeederFactory::CalDAVEventFeederFactory(QObject *parent) : QObject{ parent } { }

QStringList CalDAVEventFeederFactory::configurations() const
{
    QStringList res;

    static const QRegularExpression groupRegex = QRegularExpression("^caldav[0-9]+$");

    ReadOnlyConfdSettings settings;
    const QStringList groups = settings.childGroups();

    for (const auto &group : groups) {
        if (groupRegex.match(group).hasMatch()) {
            settings.beginGroup(group);
            const bool enabled = settings.value("enabled", false).toBool();
            settings.endGroup();

            if (enabled) {
                res.push_back(group);
            }
        }
    }

    return res;
}

IDateEventFeeder *CalDAVEventFeederFactory::createFeeder(
        const QString &settingsGroup, const QDateTime &timeRangeStart,
        const QDateTime &timeRangeEnd, DateEventFeederManager *feederManager) const
{
    ReadOnlyConfdSettings settings;
    settings.beginGroup(settingsGroup);

    auto feeder = new CalDAVEventFeeder(feederManager);

    feeder->init(settingsGroup, name(), settings.value("host").toString(),
                 settings.value("path").toString(), settings.value("user").toString(),
                 settings.value("port", 0).toInt(), settings.value("useSSL", true).toBool(),
                 settings.value("interval", 300000).toInt(), timeRangeStart, timeRangeEnd);

    return feeder;
}
