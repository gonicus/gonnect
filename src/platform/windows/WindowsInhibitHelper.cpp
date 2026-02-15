#include "WindowsInhibitHelper.h"
#include "InhibitHelper.h"
#include "Application.h"
#include <QLoggingCategory>
#include <windows.h>

Q_LOGGING_CATEGORY(lcInhibit, "gonnect.session.inhibit")

InhibitHelper &InhibitHelper::instance()
{
    static InhibitHelper *_instance = nullptr;
    if (!_instance) {
        _instance = new WindowsInhibitHelper;
    }
    return *_instance;
}

bool WindowsEventFilter::nativeEventFilter(const QByteArray &eventType, void *message,
                                           long long *result)
{
    if (eventType == "windows_generic_MSG") {
        MSG *msg = static_cast<MSG *>(message);
        bool blocking = m_inhibit;

        if (msg->message == WM_QUERYENDSESSION) {
            if (blocking) {
                *result = false;
            }
            return true;
        }

        if (msg->message == WM_ENDSESSION) {
            if (blocking && msg->wParam == FALSE) {
                return true;
            }
        }
    }

    return false;
}

WindowsInhibitHelper::WindowsInhibitHelper() : InhibitHelper{}
{
}

bool WindowsInhibitHelper::inhibitActive() const
{
    return m_inhibit;
}

void WindowsInhibitHelper::inhibit(unsigned int flags, const QString &reason)
{
    Q_UNUSED(flags)
    Q_UNUSED(reason)

    if (!m_inhibit) {
        auto app = qobject_cast<Application *>(Application::instance());
        auto hwnd = app->rootWindow()->winId();
        QString msg = QObject::tr("There are still phone calls going on");
        ShutdownBlockReasonCreate(reinterpret_cast<HWND>(hwnd),
                                  reinterpret_cast<LPCWSTR>(msg.toStdString().c_str()));

        qCDebug(lcInhibit) << "logout inhibit: active";
        m_inhibit = true;
    }
}

void WindowsInhibitHelper::release()
{
    if (m_inhibit) {
        auto app = qobject_cast<Application *>(Application::instance());
        auto hwnd = app->rootWindow()->winId();
        ShutdownBlockReasonDestroy(reinterpret_cast<HWND>(hwnd));

        qCDebug(lcInhibit) << "logout inhibit: released";
        m_inhibit = false;
    }
}

void WindowsInhibitHelper::inhibitScreenSaver(const QString &applicationName, const QString &reason)
{
    // https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate
    if (!m_screenSaverIsInhibited) {
        if (SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED)
            == NULL) {
            qCWarning(lcInhibit) << "failed to set screen inhibit mode";
        } else {
            m_screenSaverIsInhibited = true;
        }
    }
}

void WindowsInhibitHelper::releaseScreenSaver()
{
    if (m_screenSaverIsInhibited) {
        if (SetThreadExecutionState(ES_CONTINUOUS) == NULL) {
            qCWarning(lcInhibit) << "failed to reset screen inhibit mode";
        } else {
            m_screenSaverIsInhibited = false;
        }
    }
}
