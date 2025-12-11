#pragma once

#include "IDateEventFeederFactory.h"

#include <QObject>

class AkonadiEventFeederFactory : public QObject, IDateEventFeederFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "AkonadiEventFeederFactory" URI
                          "de.gonicus.gonnect.AkonadiEventFeederFactory")
    Q_INTERFACES(IDateEventFeederFactory)

public:
    AkonadiEventFeederFactory(QObject *parent = nullptr);

    virtual QString name() const override { return "Akonadi"; };
    virtual QStringList configurations() const override;
    virtual IDateEventFeeder *createFeeder(const QString &settingsGroup,
                                           const QDateTime &currentTime,
                                           const QDateTime &timeRangeStart,
                                           const QDateTime &timeRangeEnd,
                                           DateEventFeederManager *feederManager) const override;
};
