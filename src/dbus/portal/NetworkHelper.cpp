#include <netdb.h>
#include <qhostaddress.h>
#include "NetworkHelper.h"
#include "NetworkMonitor.h"

Q_LOGGING_CATEGORY(lcNetwork, "gonnect.network")

#define NH_CALL_TIMEOUT 500

NetworkHelper::NetworkHelper(QObject *parent) : QObject(parent)
{
    m_portal = new OrgFreedesktopPortalNetworkMonitorInterface("org.freedesktop.portal.Desktop",
                                                               "/org/freedesktop/portal/desktop",
                                                               QDBusConnection::sessionBus(), this);
    m_portal->setTimeout(NH_CALL_TIMEOUT);

    connect(m_portal, &OrgFreedesktopPortalNetworkMonitorInterface::changed, this,
            &NetworkHelper::updateNetworkState);

    QTimer::singleShot(0, this, &NetworkHelper::updateNetworkState);
}

void NetworkHelper::updateNetworkState()
{
    QDBusPendingReply<QVariantMap> reply = m_portal->GetStatus();
    reply.waitForFinished();

    bool localNetwork = false;
    bool limitedNetwork = false;
    bool captiveNetwork = false;
    bool fullNetwork = true;

    if (reply.isError()) {
        qCCritical(lcNetwork) << "failed to query network status:" << reply.error().message();
    } else if (reply.isValid()) {
        QVariantMap res = reply.value();

        if (res.contains("connectivity")) {
            bool ok;
            unsigned connectivity = res.value("connectivity").toUInt(&ok);
            if (ok) {
                localNetwork = connectivity == 1;
                limitedNetwork = connectivity == 2;
                captiveNetwork = connectivity == 3;
                fullNetwork = connectivity == 4;
            }
        } else if (res.contains("available")) {
            fullNetwork = true;
        } else {
            qCWarning(lcNetwork) << "status request does not contain usable fields - falling back "
                                    "to 'full network reachable'";
        }
    }

    if (localNetwork != m_localNetwork) {
        m_localNetwork = localNetwork;
        emit localNetworkAvailableChanged();
    }
    if (limitedNetwork != m_limitedNetwork) {
        m_limitedNetwork = limitedNetwork;
        emit limitedNetworkAvailableChanged();
    }
    if (captiveNetwork != m_captiveNetwork) {
        m_captiveNetwork = captiveNetwork;
        emit captiveNetworkAvailableChanged();
    }
    if (fullNetwork != m_fullNetwork) {
        m_fullNetwork = fullNetwork;
        emit fullNetworkAvailableChanged();
    }

    emit connectivityChanged();
}

bool NetworkHelper::isReachable(const QUrl &url)
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

QStringList NetworkHelper::nameservers() const
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
