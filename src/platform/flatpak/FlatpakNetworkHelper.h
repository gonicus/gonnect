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

    bool isReachable(const QUrl &url) override;

    QStringList nameservers() const override;

private:
    OrgFreedesktopPortalNetworkMonitorInterface *m_portal = nullptr;

    void updateNetworkState();
};
