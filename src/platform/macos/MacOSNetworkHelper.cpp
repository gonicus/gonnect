#include "MacOSNetworkHelper.h"

Q_DECLARE_LOGGING_CATEGORY(lcNetwork)

NetworkHelper &NetworkHelper::instance()
{
    static NetworkHelper *_instance = nullptr;
    if (!_instance) {
        _instance = new MacOSNetworkHelper;
    }
    return *_instance;
}

MacOSNetworkHelper::MacOSNetworkHelper() : NetworkHelper{} { }
