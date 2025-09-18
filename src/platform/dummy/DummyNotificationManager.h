#pragma once

#include <QObject>
#include <QUuid>
#include "NotificationManager.h"
#include "Notification.h"

class DummyNotificationManager : public NotificationManager
{
    Q_OBJECT
    Q_DISABLE_COPY(DummyNotificationManager)

public:
    explicit DummyNotificationManager() : NotificationManager() {}
    ~DummyNotificationManager() = default;

    QString add(Notification *notification) override { Q_UNUSED(notification); return QUuid::createUuid().toString(QUuid::WithoutBraces); }
    bool remove(const QString &id) override { Q_UNUSED(id); return true; }

    void shutdown() override {}
};
