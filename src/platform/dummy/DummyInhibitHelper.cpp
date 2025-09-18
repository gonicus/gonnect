#include "DummyInhibitHelper.h"

InhibitHelper &InhibitHelper::instance()
{
    static InhibitHelper *_instance = nullptr;
    if (!_instance) {
        _instance = new DummyInhibitHelper;
    }
    return *_instance;
}
