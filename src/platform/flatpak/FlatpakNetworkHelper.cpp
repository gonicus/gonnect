#include "FlatpakNetworkHelper.h"
#include "NetworkMonitor.h"
#include <qhostaddress.h>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QFuture>
#include <QLoggingCategory>
#include <QPromise>

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
    if (m_statusWatcher) {
        m_isStatusUpdatePending = true;
        return;
    }

    m_statusWatcher = new QDBusPendingCallWatcher(m_portal->GetStatus(), this);

    connect(m_statusWatcher, &QDBusPendingCallWatcher::finished, this,
            [this](QDBusPendingCallWatcher *watcher) {
                const QDBusPendingReply<QVariantMap> reply = *watcher;

                m_statusWatcher = nullptr;
                watcher->deleteLater();

                if (reply.isError()) {
                    qCWarning(lcNetwork)
                            << "failed to query network status:" << reply.error().message();
                } else if (!reply.isValid()) {
                    qCWarning(lcNetwork) << "network status is not valid - skipping";
                } else {
                    applyNetworkStatus(reply.value());
                }

                if (m_isStatusUpdatePending) {
                    m_isStatusUpdatePending = false;
                    updateNetworkState();
                }
            });
}

void FlatpakNetworkHelper::applyNetworkStatus(const QVariantMap &status)
{
    bool connected = false;

    if (status.contains("connectivity")) {
        bool ok = false;
        const unsigned connectivity = status.value("connectivity").toUInt(&ok);
        if (!ok) {
            qCCritical(lcNetwork)
                    << "error parsing unsigned integer connectivity status from portal";
            return;
        }

        connected = !!connectivity;
    } else if (status.contains("available")) {
        connected = status.value("available").toBool();
    } else {
        qCWarning(lcNetwork) << "status request does not contain usable fields - skipping";
        return;
    }

    const bool statusChanged = connected != m_connectivity;
    m_connectivity = connected;

    // Also emit signal if connected has not changed which happens e.g. WIFI -> LAN
    if (statusChanged || connected) {
        Q_EMIT connectivityChanged();
        qCDebug(lcNetwork) << "network available changed to" << connected;
    }
}

QFuture<bool> FlatpakNetworkHelper::isReachable(const QUrl &url)
{
    auto promise = std::make_shared<QPromise<bool>>();
    promise->start();
    QFuture<bool> future = promise->future();

    const int port = getStandardPort(url);
    if (port < 0) {
        qCCritical(lcNetwork) << "Cannot find standard port for" << url;

        QTimer::singleShot(0, this, [promise]() {
            promise->addResult(false);
            promise->finish();
        });

        return future;
    }

    auto *watcher = new QDBusPendingCallWatcher(
            m_portal->CanReach(url.host(), static_cast<uint>(port)), this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [promise, url](QDBusPendingCallWatcher *callWatcher) {
                const QDBusPendingReply<bool> reply = *callWatcher;

                callWatcher->deleteLater();

                bool isReachable = false;

                if (reply.isError()) {
                    qCWarning(lcNetwork) << "failed to call CanReach:" << reply.error().message();
                } else {
                    isReachable = reply.value();

                    if (!isReachable) {
                        qCWarning(lcNetwork) << "unable to reach" << url.toString();
                    }
                }

                promise->addResult(isReachable);
                promise->finish();
            });

    return future;
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
