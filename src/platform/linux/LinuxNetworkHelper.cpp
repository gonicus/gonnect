#include "LinuxNetworkHelper.h"

Q_DECLARE_LOGGING_CATEGORY(lcNetwork)

NetworkHelper &NetworkHelper::instance()
{
    static NetworkHelper *_instance = nullptr;
    if (!_instance) {
        _instance = new LinuxNetworkHelper;
    }
    return *_instance;
}

LinuxNetworkHelper::LinuxNetworkHelper() : NetworkHelper{} {}

bool LinuxNetworkHelper::isReachable(const QUrl &url) {
    Q_UNUSED(url)

    // TODO

    return true;
}
