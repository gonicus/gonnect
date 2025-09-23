#include "DummyExternalMediaManager.h"

ExternalMediaManager &ExternalMediaManager::instance()
{
    static ExternalMediaManager *_instance = nullptr;
    if (!_instance) {
        _instance = new DummyExternalMediaManager;
    }
    return *_instance;
}
