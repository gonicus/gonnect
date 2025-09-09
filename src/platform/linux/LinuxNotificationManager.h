#pragma once

#include <QObject>
#include "NotificationManager.h"
#include "Notification.h"

struct _NotifyNotification;

class LinuxNotificationManager : public NotificationManager
{
    Q_OBJECT
    Q_DISABLE_COPY(LinuxNotificationManager)

public:
    explicit LinuxNotificationManager();
    ~LinuxNotificationManager() = default;

    QString add(Notification *notification) override;
    bool remove(const QString &id) override;

    void shutdown() override;

private:
    void handleAction(QString id, QString action, QVariantList parameters);

    QMap<QString, Notification *> m_notifications;
    QMap<QString, _NotifyNotification *> m_internalNotifications;
};
