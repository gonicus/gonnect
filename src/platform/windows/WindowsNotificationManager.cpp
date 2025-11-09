#include <QApplication>
#include "WindowsNotificationManager.h"
#include <wintoastlib.h>

using namespace WinToastLib;

class WindowsNotification : public IWinToastHandler
{
public:
    WindowsNotification(WindowsNotificationManager &manager, ::Notification *notification)
        : m_manager(manager), m_notification(notification)
    {
    }

    ~WindowsNotification() { 
        m_notification->deleteLater();
    }

    void toastActivated() const override { 
        if (!m_notification->defaultAction().isEmpty()) {
            Q_EMIT m_notification->actionInvoked(m_notification->defaultAction(),
                                                 m_notification->defaultActionParameters());
        }
    }

    void toastActivated(int actionIndex) const override {        

        auto descs = m_notification->buttonDescriptions();

        if (actionIndex >= descs.count()) { 
            qCritical() << "Invalid action with index" << actionIndex;
            return;
        }

        Q_EMIT m_notification->actionInvoked(descs[actionIndex]["action"].toString(),
                                             descs[actionIndex]["target"].toList());        
    }

    void toastActivated(std::wstring response) const override {
    }

    void toastDismissed(WinToastDismissalReason state) const override {
    }

    void toastFailed() const override {
    }

    WindowsNotificationManager& m_manager;
    ::Notification* m_notification = nullptr;
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


    WinToast::instance()->setAppName(L"Gonnect");
    const auto aumi = WinToast::configureAUMI(L"mohabouje", L"wintoast", L"wintoastexample", L"20161006");
    WinToast::instance()->setAppUserModelId(aumi);
    WinToast::instance()->initialize();
}

QString WindowsNotificationManager::add(::Notification *notification)
{
    if (!notification) {
        return "";
    }

    auto windowNotification = new WindowsNotification(*this, notification);
    WinToastTemplate templ = WinToastTemplate(WinToastTemplate::Text02);
    templ.setTextField(notification->title().toStdWString(), WinToastTemplate::FirstLine);
    templ.setTextField(notification->body().toStdWString(), WinToastTemplate::SecondLine);
    auto buttonDescriptions = notification->buttonDescriptions();
    for (auto &buttonDesc : std::as_const(buttonDescriptions)) {
        if (!buttonDesc.contains("label") || !buttonDesc.contains("action")) {
            continue;
        }

        const QString label = buttonDesc.value("label").toString();
        const QString action = buttonDesc.value("action").toString();
        templ.addAction(label.toStdWString());
    }

    const auto toastId = WinToast::instance()->showToast(templ, windowNotification);
    if (toastId < 0) {
        qCritical() << "Failed to show toast";
        return {};
    }

    m_activeNotifications[toastId] = windowNotification;
    return QString("%1").arg(toastId);
}

bool WindowsNotificationManager::remove(const QString &id)
{
    const auto id64 = id.toLongLong();
    auto* windowsNotification = m_activeNotifications.value(id64);
    if (!windowsNotification) {
        return false;
    }
    WinToast::instance()->hideToast(id64);    
    delete windowsNotification;
    return true;
}

void WindowsNotificationManager::shutdown()
{
}
