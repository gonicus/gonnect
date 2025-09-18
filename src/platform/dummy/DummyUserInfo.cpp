#include "DummyUserInfo.h"

UserInfo &UserInfo::instance()
{
    static UserInfo *_instance = nullptr;
    if (!_instance) {
        _instance = new DummyUserInfo;
    }
    return *_instance;
}
