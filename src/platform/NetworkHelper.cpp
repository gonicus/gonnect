#include "NetworkHelper.h"
#include <QLoggingCategory>
#include <QFile>
#include <QUrl>
#include <QHostAddress>
#include <QTcpSocket>

Q_LOGGING_CATEGORY(lcNetwork, "gonnect.network")

NetworkHelper::NetworkHelper(QObject *parent) : QObject(parent)
{
    if (!QNetworkInformation::loadDefaultBackend()) {
        qCWarning(lcNetwork) << "QNetworkInformation is not supported on this platform or backend";
        return;
    }

    QNetworkInformation *netInfo = QNetworkInformation::instance();

    connect(netInfo, &QNetworkInformation::reachabilityChanged, this,
            &NetworkHelper::onReachabilityChanged);

    onReachabilityChanged(netInfo->reachability());
}

bool NetworkHelper::isReachable(const QUrl &url)
{
    QTcpSocket testSocket;
    testSocket.connectToHost(url.host(), url.port());
    testSocket.waitForConnected(1000);

    bool state = testSocket.state() == QTcpSocket::UnconnectedState;
    testSocket.close();

    if (!state) {
        qCWarning(lcNetwork) << url << "is not reachable";
    }

    return state;
}

void NetworkHelper::onReachabilityChanged(QNetworkInformation::Reachability reachability)
{
    bool connected = false;

    switch (reachability) {
    case QNetworkInformation::Reachability::Unknown:
        qCDebug(lcNetwork) << "network state is unknown";
        break;
    case QNetworkInformation::Reachability::Disconnected:
        qCDebug(lcNetwork) << "network disconnected";
        break;
    case QNetworkInformation::Reachability::Local:
        qCDebug(lcNetwork) << "local network locally reachable";
        connected = true;
        break;
    case QNetworkInformation::Reachability::Site:
        qCDebug(lcNetwork) << "site network is reachable";
        connected = true;
        break;
    case QNetworkInformation::Reachability::Online:
        qCDebug(lcNetwork) << "network is online.";
        connected = true;
        break;
    }

    if (m_connectivity != connected) {
        m_connectivity = connected;
        Q_EMIT connectivityChanged();
    }
}

QStringList NetworkHelper::parseResolvConf(const QString &resolvConf) const
{
    QStringList servers;
    QFile resolvconf;
    resolvconf.setFileName(resolvConf);

    if (!resolvconf.open(QIODevice::ReadOnly)) {
        return servers;
    }

    while (!resolvconf.atEnd()) {
        const QByteArray lineArray = resolvconf.readLine();
        QByteArrayView line = QByteArrayView(lineArray).trimmed();

        constexpr QByteArrayView nameserverWithSpace = "nameserver ";
        if (line.startsWith(nameserverWithSpace)) {
            auto entry = line.mid(nameserverWithSpace.size()).trimmed().toByteArray();
            QHostAddress address(entry);
            if (!address.isNull()) {
                servers.push_back(entry);
            }
        }
    }

    return servers;
}

QStringList NetworkHelper::nameservers() const
{
    QStringList result;

    result.append(parseResolvConf("/etc/resolv.conf"));
    result.append(parseResolvConf("/run/systemd/resolve/resolv.conf"));
    result.removeDuplicates();

    return result;
}
