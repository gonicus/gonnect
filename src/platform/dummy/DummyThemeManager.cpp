#include "DummyThemeManager.h"

ThemeManager &ThemeManager::instance()
{
    static ThemeManager *_instance = nullptr;
    if (!_instance) {
        _instance = new DummyThemeManager;
    }
    return *_instance;
}

DummyThemeManager::DummyThemeManager() : ThemeManager()
{
    initColorThemeDetection();
}
