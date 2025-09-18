#pragma once
#include <QObject>
#include "../NetworkHelper.h"

class DummyNetworkHelper : public NetworkHelper
{
    Q_OBJECT
    Q_DISABLE_COPY(DummyNetworkHelper)

public:
    explicit DummyNetworkHelper() : NetworkHelper() {}
    ~DummyNetworkHelper() = default;

    bool hasConnectivity() const override
    {
        return true;
    }
    bool localNetworkAvailable() const override { return true; }
    bool limitedNetworkAvailable() const override { return true; }
    bool captiveNetworkAvailable() const override { return true; }
    bool fullNetworkAvailable() const override { return true; }

    bool isReachable(const QUrl &url) override { Q_UNUSED(url); return true; }

    QStringList nameservers() const override { return QStringList(); }
};
