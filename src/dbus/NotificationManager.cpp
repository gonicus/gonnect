#include "NotificationManager.h"
#include "NotificationIcon.h"

NotificationManager::NotificationManager(QObject *parent) : QObject(parent)
{
    m_interface = new OrgFreedesktopPortalNotificationInterface(
            "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
            QDBusConnection::sessionBus(), this);

    qDBusRegisterMetaType<QList<QVariantMap>>();
    qDBusRegisterMetaType<NotificationIcon>();

    connect(m_interface, &OrgFreedesktopPortalNotificationInterface::ActionInvoked, this,
            &NotificationManager::handleAction);
}

void NotificationManager::handleAction(QString id, QString action, QVariantList parameters)
{
    if (auto notification = m_notifications.value(id, nullptr)) {
        emit notification->actionInvoked(action, parameters);
    }
}

QString NotificationManager::add(Notification *notification)
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

bool NotificationManager::remove(const QString &id)
{
    if (m_notifications.contains(id)) {
        m_notifications.value(id)->deleteLater();
        m_interface->RemoveNotification(id);
    }

    return m_notifications.remove(id);
}

void NotificationManager::shutdown()
{
    // Close notifications
    for (auto iter = m_notifications.constBegin(); iter != m_notifications.constEnd(); ++iter) {
        m_interface->RemoveNotification(iter.key());
    }

    qDeleteAll(m_notifications);
    m_notifications.clear();
}
