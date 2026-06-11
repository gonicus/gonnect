#include "WindowsInhibitHelper.h"
#include "InhibitHelper.h"
#include "Application.h"
#include "GlobalCallState.h"
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
        bool blocking = InhibitHelper::instance().inhibitActive();

        if (msg->message == WM_QUERYENDSESSION) {
            if (blocking) {
                *result = false;
                return true;
            }

            return false;
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
    connect(&GlobalCallState::instance(), &GlobalCallState::globalCallStateChanged, this, [this]() {
        if (inhibitActive()) {
            inhibit(InhibitHelper::InhibitFlag::LOGOUT,
                    QObject::tr("There are still phone calls going on"));
        } else {
            release();
        }
    });
}

bool WindowsInhibitHelper::inhibitActive() const
{
    return GlobalCallState::instance().globalCallState() & ICallState::State::CallActive;
}

void WindowsInhibitHelper::inhibit(unsigned int flags, const QString &reason)
{
    Q_UNUSED(flags)

    if (!m_inhibit) {
        auto app = static_cast<Application *>(Application::instance());
        auto rootWindow = app->rootWindow();
        if (!rootWindow) {
            // Retried on the next call state change
            qCWarning(lcInhibit) << "cannot register shutdown block reason without root window";
            return;
        }

        ShutdownBlockReasonCreate(reinterpret_cast<HWND>(rootWindow->winId()),
                                  reinterpret_cast<LPCWSTR>(reason.utf16()));

        qCDebug(lcInhibit) << "logout inhibit: active";
        m_inhibit = true;
    }
}

void WindowsInhibitHelper::release()
{
    if (m_inhibit) {
        auto app = static_cast<Application *>(Application::instance());
        if (auto rootWindow = app->rootWindow()) {
            ShutdownBlockReasonDestroy(reinterpret_cast<HWND>(rootWindow->winId()));
        }

        qCDebug(lcInhibit) << "logout inhibit: released";
        m_inhibit = false;
    }
}

void WindowsInhibitHelper::inhibitScreenSaver(const QString &applicationName, const QString &reason)
{
    Q_UNUSED(applicationName)
    Q_UNUSED(reason)

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
