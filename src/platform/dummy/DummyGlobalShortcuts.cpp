#include "DummyGlobalShortcuts.h"

GlobalShortcuts &GlobalShortcuts::instance()
{
    static GlobalShortcuts *_instance = nullptr;
    if (!_instance) {
        _instance = new DummyGlobalShortcuts;
    }
    return *_instance;
}
