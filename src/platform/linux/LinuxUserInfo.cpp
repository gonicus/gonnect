#include <QLoggingCategory>
#include <pwd.h>
#include "LinuxUserInfo.h"
#include "UserInfo.h"

Q_LOGGING_CATEGORY(lcLinuxUI, "gonnect.platform.unix")

UserInfo &UserInfo::instance()
{
    static UserInfo *_instance = nullptr;
    if (!_instance) {
        _instance = new LinuxUserInfo;
    }
    return *_instance;
}

LinuxUserInfo::LinuxUserInfo() : UserInfo{}
{
}

QString LinuxUserInfo::getDisplayName()
{
    struct passwd pwd;
    struct passwd *result;
    char *buf;

    auto bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (bufsize == -1) {
        bufsize = 16384;
    }

    buf = (char*)malloc(bufsize);
    if (buf == NULL) {
        qCritical(lcLinuxUI) << "failed to allocate memory";
        return "";
    }

    auto res = getpwuid_r(getuid(), &pwd, buf, bufsize, &result);

    if (result != NULL) {
        QString displayName = pwd.pw_gecos;
        free(buf);
        return displayName;
    }

    if (res == 0) {
        qCritical(lcLinuxUI) << "user not found";
    } else {
        qCritical(lcLinuxUI) << "failed to resolve user - error:" << res;
    }
    return "";
}
