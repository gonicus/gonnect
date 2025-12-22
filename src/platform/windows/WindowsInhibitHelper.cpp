#include "WindowsInhibitHelper.h"
#include "InhibitHelper.h"
#include "InhibitPortal.h"
#include <winbase.h>

InhibitHelper &InhibitHelper::instance()
{
    static InhibitHelper *_instance = nullptr;
    if (!_instance) {
        _instance = new WindowsInhibitHelper;
    }
    return *_instance;
}

WindowsInhibitHelper::WindowsInhibitHelper() : InhibitHelper{}
{
}

void WindowsInhibitHelper::inhibit(unsigned int flags, const QString &reason)
{
    Q_UNUSED(flags)
    Q_UNUSED(reason)
    // TODO
}

void WindowsInhibitHelper::release()
{
    // TODO
}

void WindowsInhibitHelper::inhibitScreenSaver(const QString &applicationName, const QString &reason)
{
    // https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate
    if (!m_screenSaverIsInhibited) {
        if (SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED) == NULL) {
            qCWarning(lcWinInhibit) << "failed to set screen inhibit mode";
        } else {
            m_screenSaverIsInhibited = true;
        }
    }
}

void WindowsInhibitHelper::releaseScreenSaver()
{
    if (m_screenSaverIsInhibited) {
        if (SetThreadExecutionState(ES_CONTINUOUS) == NULL) {
            qCWarning(lcWinInhibit) << "failed to reset screen inhibit mode";
        } else {
            m_screenSaverIsInhibited = false;
        }
    }
}
