#include "DummyNotificationManager.h"

NotificationManager &NotificationManager::instance()
{
    static NotificationManager *_instance = nullptr;
    if (!_instance) {
        _instance = new DummyNotificationManager;
    }
    return *_instance;
}
