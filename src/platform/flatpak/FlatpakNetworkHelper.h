#pragma once
#include <QObject>
#include "../NetworkHelper.h"

class OrgFreedesktopPortalNetworkMonitorInterface;

class FlatpakNetworkHelper : public NetworkHelper
{
    Q_OBJECT
    Q_DISABLE_COPY(FlatpakNetworkHelper)

public:
    explicit FlatpakNetworkHelper();
    ~FlatpakNetworkHelper() = default;

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
    OrgFreedesktopPortalNetworkMonitorInterface *m_portal = nullptr;

    void updateNetworkState();

    bool m_localNetwork = false;
    bool m_limitedNetwork = false;
    bool m_captiveNetwork = false;
    bool m_fullNetwork = false;
};
