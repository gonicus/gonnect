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

    bool hasConnectivity() const override
    {
        return m_fullNetwork || m_limitedNetwork || m_localNetwork;
    }
    bool localNetworkAvailable() const override { return m_localNetwork; }
    bool limitedNetworkAvailable() const override { return m_limitedNetwork; }
    bool captiveNetworkAvailable() const override { return m_captiveNetwork; }
    bool fullNetworkAvailable() const override { return m_fullNetwork; }

    bool isReachable(const QUrl &url) override;

    QStringList nameservers() const override;

private:
    bool m_localNetwork = true;
    bool m_limitedNetwork = true;
    bool m_captiveNetwork = true;
    bool m_fullNetwork = true;
};
