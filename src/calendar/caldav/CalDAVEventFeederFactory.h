#pragma once

#include "IDateEventFeederFactory.h"

#include <QObject>

class CalDAVEventFeederFactory : public QObject, IDateEventFeederFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "CalDAVEventFeederFactory" URI
                          "de.gonicus.gonnect.CalDAVEventFeederFactory")
    Q_INTERFACES(IDateEventFeederFactory)

public:
    CalDAVEventFeederFactory(QObject *parent = nullptr);

    QString name() const override { return "CalDAV"; };
    QStringList configurations() const override;
    IDateEventFeeder *createFeeder(const QString &settingsGroup,
                                           const QDateTime &currentTime,
                                           const QDateTime &timeRangeStart,
                                           const QDateTime &timeRangeEnd,
                                           DateEventFeederManager *feederManager) const override;
};
