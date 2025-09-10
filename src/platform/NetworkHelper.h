#pragma once
#include <QObject>

class NetworkHelper : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(NetworkHelper)

public:
    explicit NetworkHelper(QObject *parent = nullptr) : QObject(parent) { }

    static NetworkHelper &instance();

    virtual bool hasConnectivity() const = 0;
    virtual bool localNetworkAvailable() const = 0;
    virtual bool limitedNetworkAvailable() const = 0;
    virtual bool captiveNetworkAvailable() const = 0;
    virtual bool fullNetworkAvailable() const = 0;

    virtual bool isReachable(const QUrl &url) = 0;

    virtual QStringList nameservers() const = 0;

    ~NetworkHelper() = default;

Q_SIGNALS:
    void connectivityChanged();
    void localNetworkAvailableChanged();
    void limitedNetworkAvailableChanged();
    void captiveNetworkAvailableChanged();
    void fullNetworkAvailableChanged();
};
