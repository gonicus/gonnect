#include "FlatpakNetworkHelper.h"
#include "NetworkMonitor.h"
#include <netdb.h>
#include <qhostaddress.h>

#define NH_CALL_TIMEOUT 500

Q_DECLARE_LOGGING_CATEGORY(lcNetwork)

NetworkHelper &NetworkHelper::instance()
{
    static NetworkHelper *_instance = nullptr;
    if (!_instance) {
        _instance = new FlatpakNetworkHelper;
    }
    return *_instance;
}

FlatpakNetworkHelper::FlatpakNetworkHelper() : NetworkHelper{}
{
    m_portal = new OrgFreedesktopPortalNetworkMonitorInterface("org.freedesktop.portal.Desktop",
                                                               "/org/freedesktop/portal/desktop",
                                                               QDBusConnection::sessionBus(), this);
    m_portal->setTimeout(NH_CALL_TIMEOUT);

    connect(m_portal, &OrgFreedesktopPortalNetworkMonitorInterface::changed, this,
            &FlatpakNetworkHelper::updateNetworkState);

    QTimer::singleShot(0, this, &FlatpakNetworkHelper::updateNetworkState);
}

void FlatpakNetworkHelper::updateNetworkState()
{
    QDBusPendingReply<QVariantMap> reply = m_portal->GetStatus();
    reply.waitForFinished();

    bool connected = true;

    if (reply.isError()) {
        qCCritical(lcNetwork) << "failed to query network status:" << reply.error().message();
    } else if (reply.isValid()) {
        QVariantMap res = reply.value();

        if (res.contains("connectivity")) {
            bool ok;
            unsigned connectivity = res.value("connectivity").toUInt(&ok);
            if (ok) {
                connected = !!connectivity;
            }
        } else if (res.contains("available")) {
            connected = true;
        } else {
            qCWarning(lcNetwork) << "status request does not contain usable fields - falling back "
                                    "to 'full network reachable'";
        }
    }

    if (connected != m_connectivity) {
        m_connectivity = connected;
        Q_EMIT connectivityChanged();
        qCDebug(lcNetwork) << "network available changed to" << connected;
    }
}

bool FlatpakNetworkHelper::isReachable(const QUrl &url)
{
    int port = url.port();
    if (port < 0) {

        QString scheme = url.scheme();
        struct servent *sptr = getservbyname(scheme.toStdString().c_str(), "tcp");
        if (!sptr) {
            qCCritical(lcNetwork) << "cannot map scheme" << scheme << "to port";
            return false;
        }

        port = ntohs(sptr->s_port);
    }

    auto reply = m_portal->CanReach(url.host(), port);
    reply.waitForFinished();

    if (reply.isError()) {
        qCWarning(lcNetwork) << "failed to call CanReach:", qPrintable(reply.error().message());
        return false;
    }

    if (!reply.value()) {
        qCWarning(lcNetwork) << "unable to reach" << url.toString();
    }

    return reply.value();
}

QStringList FlatpakNetworkHelper::nameservers() const
{
    QStringList servers;
    QFile resolvconf;
    resolvconf.setFileName("/etc/resolv.conf");

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
