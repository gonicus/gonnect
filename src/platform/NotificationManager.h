#pragma once

#include <QObject>
#include "Notification.h"

class NotificationManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(NotificationManager)

public:
    Q_REQUIRED_RESULT static NotificationManager &instance();

    explicit NotificationManager() : QObject() { }
    ~NotificationManager() = default;

    virtual QString add(Notification *notification) = 0;
    virtual bool remove(const QString &id) = 0;

    virtual void shutdown() = 0;
};
