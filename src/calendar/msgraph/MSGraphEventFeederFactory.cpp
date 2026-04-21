#include "MSGraphEventFeederFactory.h"
#include "ReadOnlyConfdSettings.h"
#include "MSGraphEventFeeder.h"
#include "DateEventFeederManager.h"

MSGraphEventFeederFactory::MSGraphEventFeederFactory(QObject *parent) : QObject{ parent } { }

QStringList MSGraphEventFeederFactory::configurations() const
{
    QStringList res;

    ReadOnlyConfdSettings settings;
    const auto msOAuthGroup = QStringLiteral("msoauth");
    const auto group = QStringLiteral("msgraphcalendar");
    if (settings.childGroups().contains(msOAuthGroup) && settings.childGroups().contains(group)) {
        const auto &clientIdentifier =
                settings.value(msOAuthGroup + QStringLiteral("/clientIdentifier")).toString();
        const bool enabled = settings.value(group + QStringLiteral("enabled"), true).toBool();

        if (enabled && !clientIdentifier.isEmpty()) {
            res.push_back(group);
        }
    }

    return res;
}

IDateEventFeeder *MSGraphEventFeederFactory::createFeeder(
        const QString &settingsGroup, const QDateTime &currentTime, const QDateTime &timeRangeStart,
        const QDateTime &timeRangeEnd, DateEventFeederManager *feederManager) const
{
    Q_UNUSED(currentTime);
    return new MSGraphEventFeeder(settingsGroup, timeRangeStart, timeRangeEnd, feederManager);
}
