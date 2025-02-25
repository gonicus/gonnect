#pragma once
#include <QObject>

#define NETWORK_PORTAL_INTERFACE "org.freedesktop.portal.NetworkMonitor"

class OrgFreedesktopPortalNetworkMonitorInterface;

class NetworkHelper : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool hasConnectivity READ hasConnectivity NOTIFY connectivityChanged FINAL)
    Q_PROPERTY(bool localNetworkAvailable READ localNetworkAvailable NOTIFY
                       localNetworkAvailableChanged FINAL)
    Q_PROPERTY(bool limitedNetworkAvailable READ limitedNetworkAvailable NOTIFY
                       limitedNetworkAvailableChanged FINAL)
    Q_PROPERTY(bool captiveNetworkAvailable READ captiveNetworkAvailable NOTIFY
                       captiveNetworkAvailableChanged FINAL)
    Q_PROPERTY(bool fullNetworkAvailable READ fullNetworkAvailable NOTIFY
                       fullNetworkAvailableChanged FINAL)

    Q_DISABLE_COPY(NetworkHelper)

public:
    static NetworkHelper &instance()
    {
        static NetworkHelper *_instance = nullptr;
        if (!_instance) {
            _instance = new NetworkHelper();
        }
        return *_instance;
    }

    bool hasConnectivity() const { return m_fullNetwork || m_limitedNetwork || m_localNetwork; }
    bool localNetworkAvailable() const { return m_localNetwork; }
    bool limitedNetworkAvailable() const { return m_limitedNetwork; }
    bool captiveNetworkAvailable() const { return m_captiveNetwork; }
    bool fullNetworkAvailable() const { return m_fullNetwork; }

    bool isReachable(const QUrl &url);

    QStringList nameservers() const;

    ~NetworkHelper() = default;

signals:
    void connectivityChanged();
    void localNetworkAvailableChanged();
    void limitedNetworkAvailableChanged();
    void captiveNetworkAvailableChanged();
    void fullNetworkAvailableChanged();

private:
    explicit NetworkHelper(QObject *parent = nullptr);

    void updateNetworkState();

    OrgFreedesktopPortalNetworkMonitorInterface *m_portal = nullptr;

    bool m_localNetwork = false;
    bool m_limitedNetwork = false;
    bool m_captiveNetwork = false;
    bool m_fullNetwork = false;
};
