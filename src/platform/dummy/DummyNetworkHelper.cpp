#include "DummyNetworkHelper.h"

NetworkHelper &NetworkHelper::instance()
{
    static NetworkHelper *_instance = nullptr;
    if (!_instance) {
        _instance = new DummyNetworkHelper;
    }
    return *_instance;
}
