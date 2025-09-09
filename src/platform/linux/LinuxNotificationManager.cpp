#include <libnotify/notify.h>
#include <QApplication>
#include "LinuxNotificationManager.h"

NotificationManager &NotificationManager::instance()
{
    static NotificationManager *_instance = nullptr;
    if (!_instance) {
        _instance = new LinuxNotificationManager;
    }
    return *_instance;
}

LinuxNotificationManager::LinuxNotificationManager() : NotificationManager()
{
    notify_init(qAppName().toStdString().c_str());
}

void LinuxNotificationManager::handleAction(QString id, QString action, QVariantList parameters)
{
    if (auto notification = m_notifications.value(id, nullptr)) {
        emit notification->actionInvoked(action, parameters);
    }
}

QString LinuxNotificationManager::add(Notification *notification)
{
    if (!notification) {
        return "";
    }

    QString id = notification->id();
    NotifyNotification *internalNotification;

    // Convert icon to libnotify
    if (notification->hasThemedIcon()) {
        internalNotification = notify_notification_new(
                notification->title().toStdString().c_str(),
                notification->body().toStdString().c_str(),
                notification->iconName().toStdString().c_str());
    } else {
        QByteArray iconData = notification->iconData();

        internalNotification = notify_notification_new(
                notification->title().toStdString().c_str(),
                notification->body().toStdString().c_str(),
                iconData.isEmpty() ? "dummy" : nullptr);

        if (!iconData.isEmpty()) {
            GdkPixbufLoader* loader = gdk_pixbuf_loader_new();
            gdk_pixbuf_loader_write(loader, (const guchar*)iconData.constData(), iconData.length(), NULL);
            gdk_pixbuf_loader_close(loader, NULL);

            auto icon = gdk_pixbuf_loader_get_pixbuf (loader);
            notify_notification_set_image_from_pixbuf(internalNotification, icon);
        }
    }

    // Map urgency
    NotifyUrgency urgency;
    switch (notification->urgency()) {
        case Notification::low:
            urgency = NOTIFY_URGENCY_LOW;
            break;
        case Notification::normal:
            urgency = NOTIFY_URGENCY_NORMAL;
            break;
        case Notification::high:
        case Notification::urgent:
            urgency = NOTIFY_URGENCY_CRITICAL;
    }
    notify_notification_set_urgency(internalNotification, urgency);

    // Map hints
    unsigned displayHint = notification->displayHint();
    if (displayHint) {
        if (displayHint & Notification::DisplayHint::persistent) {
            notify_notification_set_hint(internalNotification, "persistent", NULL);
        }
        if (displayHint & Notification::DisplayHint::transient) {
            notify_notification_set_hint(internalNotification, "transient", NULL);
        } else if (displayHint & Notification::DisplayHint::tray) {
            notify_notification_set_hint(internalNotification, "tray", NULL);
        }
        if (displayHint & Notification::DisplayHint::hideOnLockscreen) {
            notify_notification_set_hint(internalNotification, "hide-on-lockscreen", NULL);
                    }
        if (displayHint & Notification::DisplayHint::hideContentOnLockScreen) {
            notify_notification_set_hint(internalNotification, "hide-content-on-lockscreen", NULL);
        }
        if (displayHint & Notification::DisplayHint::showAsNew) {
            notify_notification_set_hint(internalNotification, "show-as-new", NULL);
        }
    }

    notify_notification_set_category(internalNotification, notification->category().toStdString().c_str());

    // Assemble actions
    QList<QVariantMap> buttonDescriptions = notification->buttonDescriptions();
    for (auto& bd : std::as_const(buttonDescriptions)) {
        if (!bd.contains("label") || !bd.contains("action")) {
            continue;
        }

        QString label = bd.value("label").toString();
        QString action = bd.value("action").toString();

        notify_notification_add_action(
                internalNotification,
                action.toStdString().c_str(),
                label.toStdString().c_str(),
                [](NotifyNotification* notification, char* action, gpointer user_data){
                    Notification* en = (Notification*)user_data;
                    emit en->actionInvoked(action, {});
                    notify_notification_close(notification, NULL);
                },
                (gpointer)notification,
                NULL);
    }

    notify_notification_show(internalNotification, NULL);
    m_notifications.insert(id, notification);
    m_internalNotifications.insert(id, internalNotification);

    return notification->id();
}

bool LinuxNotificationManager::remove(const QString &id)
{
    if (m_notifications.contains(id)) {
        m_notifications.value(id)->deleteLater();
        notify_notification_close(m_internalNotifications.value(id), NULL);

        m_internalNotifications.remove(id);
    }

    return m_notifications.remove(id);
}

void LinuxNotificationManager::shutdown()
{
    // Close notifications
    for (auto iter = m_internalNotifications.constBegin(); iter != m_internalNotifications.constEnd(); ++iter) {
        notify_notification_close(iter.value(), NULL);
    }

    qDeleteAll(m_notifications);
    qDeleteAll(m_internalNotifications);
    m_notifications.clear();
    m_internalNotifications.clear();
}
