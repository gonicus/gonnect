#include <QApplication>
#include <QPointer>
#include <QThread>
#include <QLoggingCategory>
#include "WindowsNotificationManager.h"
#include <wintoastlib.h>

Q_LOGGING_CATEGORY(winNotificationCat, "gonnect.notification.win")

using namespace WinToastLib;

class WindowsNotification : public IWinToastHandler, public QObject
{
public:
    WindowsNotification(WindowsNotificationManager &manager, const QString &notificationId)
        : m_manager(manager), m_notificationId(notificationId)
    {
    }

    void toastActivated() const override
    {
        auto *notification = m_manager.notification(m_notificationId);
        if (notification) {
            if (!notification->defaultAction().isEmpty()) {
                Q_EMIT notification->actionInvoked(notification->defaultAction(),
                                                   notification->defaultActionParameters());
            }
        }
    }

    void toastActivated(int actionIndex) const override
    {
        qCDebug(winNotificationCat) << "toastActivated" << m_toastId << actionIndex;

        auto *notification = m_manager.notification(m_notificationId);
        if (notification) {
            auto descs = notification->buttonDescriptions();

            if (actionIndex >= descs.count()) {
                qCCritical(winNotificationCat) << "Invalid action with index" << actionIndex;
                return;
            }

            Q_EMIT notification->actionInvoked(descs[actionIndex]["action"].toString(),
                                               descs[actionIndex]["target"].toList());
        }
    }

    void toastActivated(std::wstring response) const override { }

    void toastDismissed(WinToastDismissalReason state) const override
    {
        auto *mutableNotification = const_cast<WindowsNotification *>(this);
        mutableNotification->deleteLater();
    }

    void toastFailed() const override { qCCritical(winNotificationCat) << "toast failed"; }

    int64_t m_toastId = -1;
    QString m_notificationId;
    WindowsNotificationManager &m_manager;
};

NotificationManager &NotificationManager::instance()
{
    static NotificationManager *_instance = nullptr;
    if (!_instance) {
        _instance = new WindowsNotificationManager;
    }
    return *_instance;
}

WindowsNotificationManager::WindowsNotificationManager() : NotificationManager()
{
    if (!WinToast::instance()->isCompatible()) {
        qCritical() << "WinToast not compatible with this system";
        return;
    }

    WinToast::instance()->setAppName(L"GOnnect");
    const auto aumi = WinToast::configureAUMI(L"Gonicus", L"GOnnect", L"", L"1");
    WinToast::instance()->setAppUserModelId(aumi);
    WinToast::instance()->initialize();
}

QString WindowsNotificationManager::add(::Notification *notification)
{
    if (!notification) {
        return "";
    }

    WinToastTemplate templ(WinToastTemplate::Text02);
    templ.setTextField(notification->title().toStdWString(), WinToastTemplate::FirstLine);
    templ.setTextField(notification->body().toStdWString(), WinToastTemplate::SecondLine);

    // Add buttons
    auto buttonDescriptions = notification->buttonDescriptions();
    for (auto &buttonDesc : std::as_const(buttonDescriptions)) {
        if (!buttonDesc.contains("label") || !buttonDesc.contains("action")) {
            continue;
        }

        const QString label = buttonDesc.value("label").toString();
        const QString action = buttonDesc.value("action").toString();
        templ.addAction(label.toStdWString());
    }

    // We may set a scenario
    // templ.setScenario(WinToastLib::WinToastTemplate::Scenario::IncomingCall);

    auto windowsNotification = new WindowsNotification(*this, notification->id());
    const auto toastId = WinToast::instance()->showToast(templ, windowsNotification);
    if (toastId < 0) {
        qCCritical(winNotificationCat) << "Failed to show notification";
        delete windowsNotification;
        return notification->id();
    }

    windowsNotification->m_toastId = toastId;

    qCDebug(winNotificationCat) << "Showing notification with id" << toastId;

    m_notifications[notification->id()] = notification;
    m_internalNotifications[notification->id()] = windowsNotification;
    return notification->id();
}

bool WindowsNotificationManager::remove(const QString &id)
{
    if (m_notifications.contains(id)) {
        m_notifications.value(id)->deleteLater();

        auto *windowsNotification = m_internalNotifications.value(id);
        if (windowsNotification) {
            WinToast::instance()->hideToast(windowsNotification->m_toastId);
            m_internalNotifications.remove(id);
        }
        m_internalNotifications.remove(id);
    }

    return m_notifications.remove(id);
}

::Notification *WindowsNotificationManager::notification(const QString &id)
{
    return m_notifications.value(id);
}

void WindowsNotificationManager::shutdown() { }
