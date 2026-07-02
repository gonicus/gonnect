#include "CallRoutingHelper.h"
#include "SIPCallManager.h"
#include "SIPCall.h"
#include "PhoneNumberUtil.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcCallRoutingHelper, "gonnect.app.CallRoutingHelper")

CallRoutingHelper::CallRoutingHelper(QObject *parent) : QObject{ parent } { }

QList<CallRoutingHopInfo> CallRoutingHelper::routingHopsForCall(const SIPCall &call)
{
    QList<CallRoutingHopInfo> l;
    const auto hops = call.routingHops();
    l.reserve(hops.size());

    for (const auto &hop : hops) {
        CallRoutingHopInfo hopInfo(hop);
        hopInfo.phoneNumber = PhoneNumberUtil::numberFromSipUrl(hop.uri);
        hopInfo.contact = PhoneNumberUtil::instance().contactInfoBySipUrl(hop.uri).contact;
        l.append(hopInfo);
    }

    return l;
}

QVariantList CallRoutingHelper::routingHopsForCall(const QString &accountId, int callId) const
{
    if (const auto call = SIPCallManager::instance().findCall(accountId, callId)) {
        const auto hops = routingHopsForCall(*call);
        QVariantList result;
        result.reserve(hops.size());

        for (const auto &hop : hops) {
            QVariantMap map;
            map["phoneNumber"] = hop.phoneNumber;
            map["reasonText"] = hop.reasonText;
            map["contactName"] = hop.contactName();
            result.append(map);
        }

        return result;
    }
    return {};
}
