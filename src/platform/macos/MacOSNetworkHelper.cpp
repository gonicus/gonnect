#include "MacOSNetworkHelper.h"
#include <QLoggingCategory>
#include <QFile>
#include <netdb.h>
#include <qhostaddress.h>

Q_LOGGING_CATEGORY(lcNetwork, "gonnect.network")

NetworkHelper &NetworkHelper::instance()
{
    static NetworkHelper *_instance = nullptr;
    if (!_instance) {
        _instance = new MacOSNetworkHelper;
    }
    return *_instance;
}

MacOSNetworkHelper::MacOSNetworkHelper() : NetworkHelper{}
{
}

bool MacOSNetworkHelper::isReachable(const QUrl &url)
{
//    int port = url.port();
//    if (port < 0) {
//
//        QString scheme = url.scheme();
//        struct servent *sptr = getservbyname(scheme.toStdString().c_str(), "tcp");
//        if (!sptr) {
//            qCCritical(lcNetwork) << "cannot map scheme" << scheme << "to port";
//            return false;
//        }
//
//        port = ntohs(sptr->s_port);
//    }
//
//    auto reply = m_portal->CanReach(url.host(), port);
//    reply.waitForFinished();
//
//    if (reply.isError()) {
//        qCWarning(lcNetwork) << "failed to call CanReach:", qPrintable(reply.error().message());
//        return false;
//    }
//
//    if (!reply.value()) {
//        qCWarning(lcNetwork) << "unable to reach" << url.toString();
//    }

    return true;
}

QStringList MacOSNetworkHelper::nameservers() const
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
