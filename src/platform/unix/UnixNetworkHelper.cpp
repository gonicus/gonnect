#include "UnixNetworkHelper.h"
#include "NetworkMonitor.h"
#include <QLoggingCategory>
#include <netdb.h>
#include <qhostaddress.h>

Q_LOGGING_CATEGORY(lcNetwork, "gonnect.network")

NetworkHelper &NetworkHelper::instance()
{
    static NetworkHelper *_instance = nullptr;
    if (!_instance) {
        _instance = new UnixNetworkHelper;
    }
    return *_instance;
}

UnixNetworkHelper::UnixNetworkHelper() : NetworkHelper{} { }

QStringList UnixNetworkHelper::nameservers() const
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
