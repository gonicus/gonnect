#pragma once
#include <QObject>
#include "../NetworkHelper.h"

class LinuxNetworkHelper : public NetworkHelper
{
    Q_OBJECT
    Q_DISABLE_COPY(LinuxNetworkHelper)

public:
    explicit LinuxNetworkHelper();

    bool isReachable(const QUrl &url) override;

    ~LinuxNetworkHelper() = default;
};
