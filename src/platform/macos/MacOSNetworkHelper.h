#pragma once
#include <QObject>
#include "../NetworkHelper.h"

class MacOSNetworkHelper : public NetworkHelper
{
    Q_OBJECT
    Q_DISABLE_COPY(MacOSNetworkHelper)

public:
    explicit MacOSNetworkHelper();
    ~MacOSNetworkHelper() = default;

    bool isReachable(const QUrl &url) override;
};
