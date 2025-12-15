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

    virtual QString name() const override { return "CalDAV"; };
    virtual QStringList configurations() const override;
    virtual IDateEventFeeder *createFeeder(const QString &settingsGroup,
                                           const QDateTime &currentTime,
                                           const QDateTime &timeRangeStart,
                                           const QDateTime &timeRangeEnd,
                                           DateEventFeederManager *feederManager) const override;
};
