#include <QLoggingCategory>
#include <QTimer>
#include <pjsua2.hpp>

#include "SIPUserAgentConfig.h"
#include "SIPManager.h"
#include "ReadOnlyConfdSettings.h"
#include "appversion.h"
#include "NetworkHelper.h"

Q_LOGGING_CATEGORY(lcSIPUserAgentConfig, "gonnect.sip.config.ua")

using std::chrono::seconds;
using namespace std::chrono_literals;

SIPUserAgentConfig::SIPUserAgentConfig(SIPManager *parent) : QObject(parent) { }

void SIPUserAgentConfig::applyConfig(pj::EpConfig &epConfig)
{
    ReadOnlyConfdSettings settings;
    bool ok = false;

    epConfig.uaConfig.userAgent =
            QString("GOnnect v%1").arg(QString::fromStdString(getVersion())).toStdString();
    epConfig.uaConfig.threadCnt = 0;
    epConfig.uaConfig.mainThreadOnly = true;

    bool stunTryIpv6 = settings.value("ua/stunTryIpv6", false).toBool();
    epConfig.uaConfig.stunTryIpv6 = stunTryIpv6;

    bool stunIgnoreFailure = settings.value("ua/stunIgnoreFailure", false).toBool();
    epConfig.uaConfig.stunIgnoreFailure = stunIgnoreFailure;

    bool mwiUnsolicitedEnabled = settings.value("ua/mwiUnsolicitedEnabled", true).toBool();
    epConfig.uaConfig.mwiUnsolicitedEnabled = mwiUnsolicitedEnabled;

    // https://docs.pjsip.org/en/latest/api/generated/pjsip/group/group__PJSUA2__UA.html#_CPPv4N2pj8UaConfig10enableUpnpE
    bool enableUpnp = settings.value("ua/enableUpnp", false).toBool();
    epConfig.uaConfig.enableUpnp = enableUpnp;

    QString upnpIfName = settings.value("ua/upnpIfName", "").toString();
    epConfig.uaConfig.upnpIfName = upnpIfName.toStdString();

    unsigned maxCalls = settings.value("ua/maxCalls", PJSUA_MAX_CALLS).toUInt(&ok);
    if (ok) {
        epConfig.uaConfig.maxCalls = maxCalls;
    } else {
        qCWarning(lcSIPUserAgentConfig) << "invalid value for maxCalls, using default";
        epConfig.uaConfig.maxCalls = PJSUA_MAX_CALLS;
    }

    // https://docs.pjsip.org/en/latest/api/generated/pjsip/group/group__PJSUA2__UA.html#_CPPv4N2pj8UaConfig12natTypeInSdpE
    unsigned natTypeInSdp = settings.value("ua/natTypeInSdp", 1).toUInt(&ok);
    if (ok) {
        epConfig.uaConfig.natTypeInSdp = natTypeInSdp;
    } else {
        qCWarning(lcSIPUserAgentConfig) << "invalid value for natTypeInSdp, using default";
        epConfig.uaConfig.natTypeInSdp = 1;
    }

    QStringList outboundProxies = settings.value("ua/outboundProxies", {}).toStringList();
    std::vector<std::string> cnvProxies;
    for (auto iter = outboundProxies.constBegin(); iter != outboundProxies.constEnd(); ++iter) {
        cnvProxies.push_back(iter->toStdString());
    }
    epConfig.uaConfig.outboundProxies = cnvProxies;

    QStringList stunServers = settings.value("ua/stunServers", {}).toStringList();
    std::vector<std::string> cnvStunServers;
    for (auto iter = stunServers.constBegin(); iter != stunServers.constEnd(); ++iter) {
        cnvStunServers.push_back(iter->toStdString());
    }
    epConfig.uaConfig.stunServer = cnvStunServers;

    QStringList nameservers =
            settings.value("ua/nameservers", NetworkHelper::instance().nameservers())
                    .toStringList();
    std::vector<std::string> cnvNameServers;
    for (auto iter = nameservers.constBegin(); iter != nameservers.constEnd(); ++iter) {
        if (!iter->isEmpty()) {
            cnvNameServers.push_back(iter->toStdString());
        }
    }
    epConfig.uaConfig.nameserver = cnvNameServers;
}
