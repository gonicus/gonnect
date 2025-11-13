#pragma once
#include <QObject>
#include "../NetworkHelper.h"

class UnixNetworkHelper : public NetworkHelper
{
    Q_OBJECT
    Q_DISABLE_COPY(UnixNetworkHelper)

public:
    explicit UnixNetworkHelper();
    ~UnixNetworkHelper() = default;

    bool hasConnectivity() const override
    {
        return m_fullNetwork || m_limitedNetwork || m_localNetwork;
    }
    bool localNetworkAvailable() const override { return true; }
    bool limitedNetworkAvailable() const override { return true; }
    bool captiveNetworkAvailable() const override { return true; }
    bool fullNetworkAvailable() const override { return true; }

    bool isReachable(const QUrl &url) override { return true; }

    QStringList nameservers() const override;
};
