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
    char displayName[UNLEN + 1];
    unsigned len = UNLEN + 1;
    GetUserNameExA(EXTENDED_NAME_FORMAT::NameDisplay, displayName, &len);

    return QString(displayName);
}
