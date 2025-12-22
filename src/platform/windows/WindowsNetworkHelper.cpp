#include "WindowsNetworkHelper.h"
#include <winsock2.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(lcNetwork)

NetworkHelper &NetworkHelper::instance()
{
    static NetworkHelper *_instance = nullptr;
    if (!_instance) {
        _instance = new WindowsNetworkHelper();
    }
    return *_instance;
}

WindowsNetworkHelper::WindowsNetworkHelper() : NetworkHelper{} { }

QStringList WindowsNetworkHelper::nameservers() const
{
    // start with 64k
    ULONG bufferSize = 65536;
    IP_ADAPTER_ADDRESSES *addresses = nullptr;

    // Try GetAdapterAddresses up to 3 times (the size of memory needed can change during calls)
    for (int i = 0; i < 3; i++) {
        addresses = reinterpret_cast<IP_ADAPTER_ADDRESSES *>(malloc(bufferSize));
        if (!addresses) {
            qCCritical(lcNetwork) << "Failed to allocate memory for GetAdaptersAddresses";
            return {};
        }

        const auto result = GetAdaptersAddresses(AF_UNSPEC,
                                                 GAA_FLAG_SKIP_UNICAST | GAA_FLAG_SKIP_ANYCAST
                                                         | GAA_FLAG_SKIP_MULTICAST
                                                         | GAA_FLAG_SKIP_FRIENDLY_NAME,
                                                 0, // reserved
                                                 addresses, &bufferSize);

        if (result == NO_ERROR) {
            break;
        }

        if (result == ERROR_BUFFER_OVERFLOW) {
            // retry using the bufferSize returned by GetAdapterAddresses
            free(addresses);
            continue;
        }

        qCritical(lcNetwork) << "GetAdaptersAddresses failed with code" << result;
        return {};
    }

    QStringList servers;

    auto *curAddress = addresses;
    while (curAddress) {

        auto *dnsServer = curAddress->FirstDnsServerAddress;
        while (dnsServer) {
            char buf[128];
            if (dnsServer->Address.lpSockaddr->sa_family == AF_INET) {
                const auto *address =
                        reinterpret_cast<const sockaddr_in *>(dnsServer->Address.lpSockaddr);
                inet_ntop(AF_INET, &address->sin_addr, buf, sizeof(buf));
                servers << buf;
            } else if (dnsServer->Address.lpSockaddr->sa_family == AF_INET6) {
                const auto *address =
                        reinterpret_cast<const sockaddr_in6 *>(dnsServer->Address.lpSockaddr);
                inet_ntop(AF_INET6, &address->sin6_addr, buf, sizeof(buf));
                servers << buf;
            }

            dnsServer = dnsServer->Next;
        }

        curAddress = curAddress->Next;
    }

    free(addresses);

    return servers;
}
