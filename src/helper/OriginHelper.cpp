#include "OriginHelper.h"

using namespace Qt::Literals::StringLiterals;

namespace {

int defaultPort(const QUrl &url)
{
    const QString scheme = url.scheme();
    if (scheme == "https"_L1 || scheme == "wss"_L1) {
        return 443;
    }
    if (scheme == "http"_L1 || scheme == "ws"_L1) {
        return 80;
    }
    return -1;
}
} // namespace

bool OriginHelper::isSameOrigin(const QUrl &first, const QUrl &second)
{
    return !first.host().isEmpty() && first.scheme() == second.scheme()
            && first.host() == second.host()
            && first.port(defaultPort(first)) == second.port(defaultPort(second));
}
