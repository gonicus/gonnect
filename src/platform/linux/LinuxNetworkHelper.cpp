#include "LinuxNetworkHelper.h"
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(lcNetwork)

NetworkHelper &NetworkHelper::instance()
{
    static NetworkHelper *_instance = nullptr;
    if (!_instance) {
        _instance = new LinuxNetworkHelper;
    }
    return *_instance;
}

LinuxNetworkHelper::LinuxNetworkHelper() : NetworkHelper{} { }
