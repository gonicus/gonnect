#pragma once

#include "IDateEventFeederFactory.h"

#include <QObject>

class EDSEventFeederFactory : public QObject, IDateEventFeederFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "EDSEventFeederFactory" URI "de.gonicus.gonnect.EDSEventFeederFactory")
    Q_INTERFACES(IDateEventFeederFactory)

public:
    EDSEventFeederFactory(QObject *parent = nullptr);

    virtual QString name() const override { return "EDS"; };
    virtual QStringList configurations() const override;
    virtual IDateEventFeeder *createFeeder(const QString &settingsGroup,
                                           const QDateTime &currentTime,
                                           const QDateTime &timeRangeStart,
                                           const QDateTime &timeRangeEnd,
                                           DateEventFeederManager *feederManager) const override;
};
