#include "DummyPlatformSession.h"

PlatformSession &PlatformSession::instance()
{
    static PlatformSession *_instance = nullptr;
    if (!_instance) {
        _instance = new DummyPlatformSession;
    }
    return *_instance;
}
