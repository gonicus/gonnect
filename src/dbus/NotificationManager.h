#pragma once

#include <QObject>
#include "NotificationInterface.h"
#include "Notification.h"

class NotificationManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(NotificationManager)

public:
    Q_REQUIRED_RESULT static NotificationManager &instance()
    {
        static NotificationManager *_instance = nullptr;

        if (_instance == nullptr) {
            _instance = new NotificationManager();
        }

        return *_instance;
    }

    ~NotificationManager() = default;

    QString add(Notification *notification);
    bool remove(const QString &id);

    void shutdown();

signals:
    void actionInvoked(QString id, QString action, QVariantList parameters);

private:
    explicit NotificationManager(QObject *parent = nullptr);

    void handleAction(QString id, QString action, QVariantList parameters);

    QMap<QString, Notification *> m_notifications;

    OrgFreedesktopPortalNotificationInterface *m_interface = nullptr;
};
