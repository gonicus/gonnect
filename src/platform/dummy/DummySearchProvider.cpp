#include "DummySearchProvider.h"

SearchProvider &SearchProvider::instance()
{
    static SearchProvider *_instance = nullptr;
    if (!_instance) {
        _instance = new DummyDesktopSearchProvider;
    }
    return *_instance;
}
