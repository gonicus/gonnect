#pragma once

#include <QtPlugin>

#define IDateEventFeederFactory_iid "de.gonicus.gonnect.IDateEventFeederFactory"

class IDateEventFeeder;
class DateEventFeederManager;

class IDateEventFeederFactory
{

public:
    virtual QString name() const = 0;
    virtual QStringList configurations() const = 0;
    virtual IDateEventFeeder *createFeeder(const QString &settingsGroup,
                                           const QDateTime &currentTime,
                                           const QDateTime &timeRangeStart,
                                           const QDateTime &timeRangeEnd,
                                           DateEventFeederManager *feederManager) const = 0;
};

Q_DECLARE_INTERFACE(IDateEventFeederFactory, IDateEventFeederFactory_iid)
