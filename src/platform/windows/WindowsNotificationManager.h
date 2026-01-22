#pragma once

#include <QObject>
#include <QMap>
#include "NotificationManager.h"
#include "Notification.h"

class WindowsNotification;

class WindowsNotificationManager : public NotificationManager
{
    Q_OBJECT
    Q_DISABLE_COPY(WindowsNotificationManager)

public:
    explicit WindowsNotificationManager();
    ~WindowsNotificationManager() = default;

    QString add(Notification *notification) override;
    bool remove(const QString &id) override;
    Notification *notification(const QString &id);

    void shutdown() override;

private:
    QMap<QString, Notification *> m_notifications;

    QMap<QString, WindowsNotification *> m_internalNotifications;
};
