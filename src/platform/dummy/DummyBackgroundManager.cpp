#include "DummyBackgroundManager.h"

BackgroundManager &BackgroundManager::instance()
{
    static BackgroundManager *_instance = nullptr;
    if (!_instance) {
        _instance = new DummyBackgroundManager;
    }
    return *_instance;
}
