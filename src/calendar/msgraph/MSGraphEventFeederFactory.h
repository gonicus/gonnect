#pragma once

#include "IDateEventFeederFactory.h"

#include <QObject>

class MSGraphEventFeederFactory : public QObject, IDateEventFeederFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "MSGraphEventFeederFactory" URI
                          "de.gonicus.gonnect.MSGraphEventFeederFactory")
    Q_INTERFACES(IDateEventFeederFactory)

public:
    MSGraphEventFeederFactory(QObject *parent = nullptr);

    QString name() const override { return "MSGraphCalendar"; };
    QStringList configurations() const override;
    IDateEventFeeder *createFeeder(const QString &settingsGroup, const QDateTime &currentTime,
                                   const QDateTime &timeRangeStart, const QDateTime &timeRangeEnd,
                                   const int retryCount, const int retryInterval,
                                   DateEventFeederManager *feederManager) const override;
};
