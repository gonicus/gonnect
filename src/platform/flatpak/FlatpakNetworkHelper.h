#pragma once
#include <QObject>
#include <QVariantMap>
#include "../NetworkHelper.h"

class OrgFreedesktopPortalNetworkMonitorInterface;
class QDBusPendingCallWatcher;

class FlatpakNetworkHelper : public NetworkHelper
{
    Q_OBJECT
    Q_DISABLE_COPY(FlatpakNetworkHelper)

public:
    explicit FlatpakNetworkHelper();
    ~FlatpakNetworkHelper() = default;

    QFuture<bool> isReachable(const QUrl &url) override;

    QStringList nameservers() const override;

private:
    OrgFreedesktopPortalNetworkMonitorInterface *m_portal = nullptr;
    QDBusPendingCallWatcher *m_statusWatcher = nullptr;
    bool m_isStatusUpdatePending = false;

    void updateNetworkState();
    void applyNetworkStatus(const QVariantMap &status);
};
