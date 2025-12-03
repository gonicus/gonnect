#include <QLoggingCategory>
#include <windows.h>
#include <Lmcons.h>
#include "WindowsUserInfo.h"
#include "UserInfo.h"

Q_LOGGING_CATEGORY(lcWindowsUI, "gonnect.platform.windows.userinfo")

UserInfo &UserInfo::instance()
{
    static UserInfo *_instance = nullptr;
    if (!_instance) {
        _instance = new WindowsUserInfo;
    }
    return *_instance;
}

WindowsUserInfo::WindowsUserInfo() : UserInfo{}
{
}

QString WindowsUserInfo::getDisplayName()
{
    WCHAR displayName[UNLEN + 1];
    DWORD len = UNLEN + 1;
    if (!GetUserName(displayName, &len)) {
        qCCritical(lcWindowsUI) << "failed to acquire user name";
        return "";
    }

    return QString::fromWCharArray(displayName, len);
}
