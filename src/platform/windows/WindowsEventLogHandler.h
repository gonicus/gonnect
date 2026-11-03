#include "logfault/logfault.h"
#include "windows/EventLog.h"
#include <windows.h>
#include <string>
#include <QString>

using namespace logfault;

class WindowsEventLogHandler : public Handler
{
public:
    WindowsEventLogHandler(const std::string &name, LogLevel level) : Handler(level)
    {
        std::wstring wide = QString::fromStdString(name).toStdWString();
        m_h = RegisterEventSource(0, wide.c_str());
    }

    ~WindowsEventLogHandler() { DeregisterEventSource(m_h); }

    void LogMessage(const logfault::Message &msg) LOGFAULT_NOEXCEPT override
    {
        if (!m_h) {
            return;
        }
        WORD wtype = EVENTLOG_SUCCESS;
        DWORD eventId = MSG_INFO_1;
        switch (msg.level_) {
        case LogLevel::ERROR:
            wtype = EVENTLOG_ERROR_TYPE;
            eventId = MSG_ERROR_1;
            break;
        case LogLevel::WARN:
            wtype = EVENTLOG_WARNING_TYPE;
            eventId = MSG_WARNING_1;
            break;
        default:;
        }
        std::wstring stemp = QString::fromStdString(msg.msg_).toStdWString();
        LPCWSTR buffer = reinterpret_cast<LPCWSTR>(stemp.c_str());
        ReportEventW(m_h, wtype, 0, eventId, NULL, 1, 0, &buffer, NULL);
    }

private:
    HANDLE m_h = {};
};
