#include "FlatpakNetworkHelper.h"
#include "NetworkMonitor.h"
#include <qhostaddress.h>
#include <QtConcurrent>

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

    if (reply.isError()) {
        qCWarning(lcNetwork) << "failed to query network status:" << reply.error().message();
        return;
    }

    if (!reply.isValid()) {
        qCWarning(lcNetwork) << "network status is not valid - skipping";
        return;
    }

    const QVariantMap res = reply.value();
    bool connected = false;

    if (res.contains("connectivity")) {
        bool ok = false;
        const unsigned connectivity = res.value("connectivity").toUInt(&ok);
        if (!ok) {
            qCCritical(lcNetwork)
                    << "error parsing unsigned integer connectivity status from portal";
            return;
        }

        connected = !!connectivity;
    } else if (res.contains("available")) {
        connected = res.value("available").toBool();
    } else {
        qCWarning(lcNetwork) << "status request does not contain usable fields - skipping";
        return;
    }

    if (connected != m_connectivity) {
        m_connectivity = connected;
        Q_EMIT connectivityChanged();
        qCDebug(lcNetwork) << "network available changed to" << connected;
    }
}

QFuture<bool> FlatpakNetworkHelper::isReachable(const QUrl &url)
{
    return QtConcurrent::run([url, this]() -> bool {
        const int port = getStandardPort(url);
        if (port < 0) {
            qCCritical(lcNetwork) << "Cannot find standard port for" << url;
            return false;
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
    });
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
