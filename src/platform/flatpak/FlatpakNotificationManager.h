#pragma once

#include <QObject>
#include "NotificationManager.h"
#include "NotificationInterface.h"
#include "Notification.h"

class FlatpakNotificationManager : public NotificationManager
{
    Q_OBJECT
    Q_DISABLE_COPY(FlatpakNotificationManager)

public:
    explicit FlatpakNotificationManager();
    ~FlatpakNotificationManager() = default;

    QString add(Notification *notification) override;
    bool remove(const QString &id) override;

    void shutdown() override;

private:
    void handleAction(QString id, QString action, QVariantList parameters);

    QMap<QString, Notification *> m_notifications;

    OrgFreedesktopPortalNotificationInterface *m_interface = nullptr;
};
