#include "FlatpakNotificationManager.h"
#include "NotificationIcon.h"

NotificationManager &NotificationManager::instance()
{
    static NotificationManager *_instance = nullptr;
    if (!_instance) {
        _instance = new FlatpakNotificationManager;
    }
    return *_instance;
}

FlatpakNotificationManager::FlatpakNotificationManager() : NotificationManager()
{
    m_interface = new OrgFreedesktopPortalNotificationInterface(
            "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
            QDBusConnection::sessionBus(), this);

    qDBusRegisterMetaType<QList<QVariantMap>>();
    qDBusRegisterMetaType<NotificationIcon>();

    connect(m_interface, &OrgFreedesktopPortalNotificationInterface::ActionInvoked, this,
            &FlatpakNotificationManager::handleAction);
}

void FlatpakNotificationManager::handleAction(QString id, QString action, QVariantList parameters)
{
    if (auto notification = m_notifications.value(id, nullptr)) {
        emit notification->actionInvoked(action, parameters);
    }
}

QString FlatpakNotificationManager::add(Notification *notification)
{
    if (!notification) {
        return "";
    }

    QString id = notification->id();
    notification->setVersion(m_interface->version());
    m_interface->AddNotification(id, notification->toPortalDefinition());
    m_notifications.insert(id, notification);

    return notification->id();
}

bool FlatpakNotificationManager::remove(const QString &id)
{
    if (m_notifications.contains(id)) {
        m_notifications.value(id)->deleteLater();
        m_interface->RemoveNotification(id);
    }

    return m_notifications.remove(id);
}

void FlatpakNotificationManager::shutdown()
{
    // Close notifications
    for (auto iter = m_notifications.constBegin(); iter != m_notifications.constEnd(); ++iter) {
        m_interface->RemoveNotification(iter.key());
    }

    qDeleteAll(m_notifications);
    m_notifications.clear();
}
