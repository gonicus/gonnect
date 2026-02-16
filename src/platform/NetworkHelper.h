#pragma once
#include <QObject>
#include <QNetworkInformation>
#include <QFuture>
#include <QJsonDocument>

class NetworkHelper : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(NetworkHelper)

public:
    static QFuture<QString> fetchUrlAsString(const QUrl &url);
    static QFuture<QJsonDocument> fetchUrlAsJson(const QUrl &url);

    explicit NetworkHelper(QObject *parent = nullptr);

    static NetworkHelper &instance();

    virtual bool hasConnectivity() const { return m_connectivity; }
    virtual bool isReachable(const QUrl &url);

    virtual QStringList nameservers() const;

    ~NetworkHelper() = default;

Q_SIGNALS:
    void connectivityChanged();

protected:
    bool m_connectivity = false;

private:
    void onReachabilityChanged(QNetworkInformation::Reachability reachability);

    QStringList parseResolvConf(const QString &resolvConf) const;
};
