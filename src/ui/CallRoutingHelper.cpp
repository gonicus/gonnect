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

QList<CallRoutingHopInfo> CallRoutingHelper::routingHopsForCall(const QString &accountId,
                                                                int callId) const
{
    const auto call = SIPCallManager::instance().findCall(accountId, callId);
    if (!call) {
        qCCritical(lcCallRoutingHelper)
                << "Unable to find call" << callId << "for account" << accountId;
        return {};
    }

    return routingHopsForCall(*call);
}
