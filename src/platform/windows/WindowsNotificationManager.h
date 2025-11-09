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

    void shutdown() override;
    
private:
    QMap<int64_t, WindowsNotification*> m_activeNotifications;
};
