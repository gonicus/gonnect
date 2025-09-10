#include <QLoggingCategory>
#include <QRegularExpression>
#include "SIPAccount.h"
#include "SIPManager.h"
#include "SIPCallManager.h"
#include "SIPBuddy.h"
#include "PreferredIdentity.h"
#include "PhoneNumberUtil.h"
#include "ErrorBus.h"
#include "KeychainSettings.h"
#include "NetworkHelper.h"
#include "Credentials.h"

#include <QUuid>

Q_LOGGING_CATEGORY(lcSIPAccount, "gonnect.sip.account")

long SIPAccount::runningMessageIndex = 0;

SIPAccount::SIPAccount(const QString &group, QObject *parent)
    : QObject(parent), Account(), m_account(group)
{
}

void SIPAccount::initialize()
{
    bool ok = false;
    static QRegularExpression sipURI = QRegularExpression("^(sips?):([^@]+)(?:@(.+))?$");

    m_settings.beginGroup(m_account);

    m_shallNegotiateCapabilities = m_settings.value("negotiateCapabilities", true).toBool();
    m_useInstantMessagingWithoutCheck =
            m_settings.value("useInstantMessagingWithoutCheck", false).toBool();

    QString transport = m_settings.value("transport", "tls").toString();
    if (transport == "tls") {
        m_transportType = TRANSPORT_TYPE::TLS;
    } else if (transport == "udp") {
        m_transportType = TRANSPORT_TYPE::UDP;
    } else if (transport == "tcp") {
        m_transportType = TRANSPORT_TYPE::TCP;
    } else {
        qCCritical(lcSIPAccount) << "invalid transport [tcp, udp, tls]:" << transport;
        Q_EMIT initialized(false);
        return;
    }

    QString net = m_settings.value("network", "auto").toString();
    if (net == "auto") {
        m_transportNet = TRANSPORT_NET::AUTO;
    } else if (net == "ipv4") {
        m_transportNet = TRANSPORT_NET::IPv4;
    } else if (net == "ipv6") {
        m_transportNet = TRANSPORT_NET::IPv6;
    } else {
        qCCritical(lcSIPAccount) << "invalid transport [auto, ipv4, ipv6]:" << transport;
        Q_EMIT initialized(false);
        return;
    }

    QString userUri = m_settings.value("userUri", "").toString();
    if (!userUri.isEmpty()) {
        if (!sipURI.match(userUri).hasMatch()) {
            qCCritical(lcSIPAccount) << "'userUri' is no valid SIP URI:" << userUri;
            ErrorBus::instance().addFatalError(
                    tr("'userUri' is no valid SIP URI: %1").arg(userUri));
            Q_EMIT initialized(false);
            return;
        }

        m_accountConfig.idUri = userUri.toStdString();
    } else {
        qCCritical(lcSIPAccount) << "'userUri' is required";
        ErrorBus::instance().addFatalError(tr("'userUri' is required"));
        Q_EMIT initialized(false);
        return;
    }

    QString registrarUri = m_settings.value("registrarUri", "").toString();
    if (!registrarUri.isEmpty()) {
        m_domain = sipURI.match(registrarUri).captured(2);

        if (!sipURI.match(registrarUri).hasMatch()) {
            qCCritical(lcSIPAccount) << "'registrarUri' is no valid SIP URI:" << registrarUri;
            ErrorBus::instance().addFatalError(
                    tr("'registrarUri' is no valid SIP URI: %1").arg(registrarUri));
            Q_EMIT initialized(false);
            return;
        }

        registrarUri = addTransport(registrarUri);

        m_accountConfig.regConfig.retryIntervalSec = 30;
        m_accountConfig.regConfig.registrarUri = registrarUri.toStdString();
    } else {
        qCCritical(lcSIPAccount) << "'registrarUri' is required";
        ErrorBus::instance().addFatalError(tr("'registrarUri' is required"));
        Q_EMIT initialized(false);
        return;
    }

    QStringList proxies = m_settings.value("proxies").toStringList();

    for (auto &proxy : std::as_const(proxies)) {
        if (!sipURI.match(proxy).hasMatch()) {
            qCCritical(lcSIPAccount) << "'proxies' contains invalid SIP URI entry:" << proxy;
            ErrorBus::instance().addFatalError(
                    tr("'proxies' contains invalid SIP URI entry: %1").arg(proxy));
            Q_EMIT initialized(false);
            return;
        }

        QString _proxy = addTransport(proxy);
        m_accountConfig.sipConfig.proxies.push_back(_proxy.toStdString());
    }

    QString srtpUse = m_settings.value("srtpUse").toString();
    pjmedia_srtp_use srtpUseValue = PJSUA_DEFAULT_USE_SRTP;
    if (!srtpUse.isEmpty()) {
        if (srtpUse == "mandatory") {
            srtpUseValue = PJMEDIA_SRTP_MANDATORY;
        } else if (srtpUse == "optional") {
            srtpUseValue = PJMEDIA_SRTP_OPTIONAL;
        } else if (srtpUse == "disabled") {
            srtpUseValue = PJMEDIA_SRTP_DISABLED;
        } else {
            qCCritical(lcSIPAccount)
                    << "invalid value for 'srtpUse' [disabled, optional, mandatory]:" << srtpUse;
            Q_EMIT initialized(false);
            return;
        }
    }
    m_accountConfig.mediaConfig.srtpUse = srtpUseValue;

    int rtpPort = m_settings.value("rtpPort", 0).toInt(&ok);
    if (!ok) {
        qCCritical(lcSIPAccount) << "invalid value for 'rtpPort':" << rtpPort;
        Q_EMIT initialized(false);
        return;
    }
    if (rtpPort != 0) {
        m_accountConfig.mediaConfig.transportConfig.port = rtpPort;
    }

    int rtpPortRange = m_settings.value("rtpPortRange", 0).toInt(&ok);
    if (!ok) {
        qCCritical(lcSIPAccount) << "invalid value for 'rtpPortRange':" << rtpPort;
        Q_EMIT initialized(false);
        return;
    }
    if (rtpPortRange != 0) {
        m_accountConfig.mediaConfig.transportConfig.portRange = rtpPortRange;
    }

    m_accountConfig.mediaConfig.transportConfig.randomizePort =
            m_settings.value("randomizeRtpPorts", false).toBool();

    // Tweak IPv6 account settings
    if (m_transportNet == TRANSPORT_NET::IPv4) {
        m_accountConfig.sipConfig.ipv6Use = PJSUA_IPV6_DISABLED;
        m_accountConfig.mediaConfig.ipv6Use = PJSUA_IPV6_DISABLED;
    } else if (m_transportNet == TRANSPORT_NET::IPv6) {
        m_accountConfig.sipConfig.ipv6Use = PJSUA_IPV6_ENABLED_USE_IPV6_ONLY;
        m_accountConfig.mediaConfig.ipv6Use = PJSUA_IPV6_ENABLED_USE_IPV6_ONLY;
    } else if (m_transportNet == TRANSPORT_NET::AUTO || m_transportNet == TRANSPORT_NET::IPv6) {
        m_accountConfig.sipConfig.ipv6Use = PJSUA_IPV6_ENABLED_PREFER_IPV6;
        m_accountConfig.mediaConfig.ipv6Use = PJSUA_IPV6_ENABLED_PREFER_IPV6;
    }

    int srtpSecureSignaling =
            m_settings.value("srtpSecureSignaling", PJSUA_DEFAULT_SRTP_SECURE_SIGNALING).toInt(&ok);
    if (!ok) {
        qCCritical(lcSIPAccount) << "invalid value for 'srtpSecureSignaling' [0, 1 ,2]:"
                                 << srtpSecureSignaling;
        Q_EMIT initialized(false);
        return;
    }
    m_accountConfig.mediaConfig.srtpSecureSignaling = srtpSecureSignaling;

    bool lockCodecEnabled = m_settings.value("lockCodecEnabled").toBool();
    m_accountConfig.mediaConfig.lockCodecEnabled = lockCodecEnabled;

    // Transport
    int port = m_settings.value("port", 5061).toInt(&ok);
    if (!ok) {
        qCCritical(lcSIPAccount) << "invalid value for 'port':" << port;
        Q_EMIT initialized(false);
        return;
    }
    m_transportConfig.port = port;

    int portRange = m_settings.value("portRange", 10).toInt(&ok);
    if (!ok) {
        qCCritical(lcSIPAccount) << "invalid value for 'portRange':" << portRange;
        Q_EMIT initialized(false);
        return;
    }
    m_transportConfig.portRange = portRange;

    m_transportConfig.randomizePort = m_settings.value("randomizePorts", false).toBool();

    QString caListFile = m_settings.value("caListFile").toString();
    if (!caListFile.isEmpty()) {
        m_transportConfig.tlsConfig.CaListFile = caListFile.toStdString();
    }

    QString certFile = m_settings.value("certFile").toString();
    if (!certFile.isEmpty()) {
        m_transportConfig.tlsConfig.certFile = certFile.toStdString();
    }

    QString privKeyFile = m_settings.value("privateKeyFile").toString();
    if (!privKeyFile.isEmpty()) {
        m_transportConfig.tlsConfig.privKeyFile = privKeyFile.toStdString();
    }

    m_transportConfig.tlsConfig.verifyServer = m_settings.value("verifyServer", false).toBool();

    try {
        if (m_transportNet == TRANSPORT_NET::AUTO || m_transportNet == TRANSPORT_NET::IPv4) {
            if (m_transportType == TRANSPORT_TYPE::TLS) {
                SIPManager::instance().endpoint().transportCreate(PJSIP_TRANSPORT_TLS,
                                                                  m_transportConfig);
            } else if (m_transportType == TRANSPORT_TYPE::TCP) {
                SIPManager::instance().endpoint().transportCreate(PJSIP_TRANSPORT_TCP,
                                                                  m_transportConfig);
            } else if (m_transportType == TRANSPORT_TYPE::UDP) {
                SIPManager::instance().endpoint().transportCreate(PJSIP_TRANSPORT_UDP,
                                                                  m_transportConfig);
            }
        }
        if (m_transportNet == TRANSPORT_NET::AUTO || m_transportNet == TRANSPORT_NET::IPv6) {
            if (m_transportType == TRANSPORT_TYPE::TLS) {
                SIPManager::instance().endpoint().transportCreate(PJSIP_TRANSPORT_TLS6,
                                                                  m_transportConfig);
            } else if (m_transportType == TRANSPORT_TYPE::TCP) {
                SIPManager::instance().endpoint().transportCreate(PJSIP_TRANSPORT_TCP6,
                                                                  m_transportConfig);
            } else if (m_transportType == TRANSPORT_TYPE::UDP) {
                SIPManager::instance().endpoint().transportCreate(PJSIP_TRANSPORT_UDP6,
                                                                  m_transportConfig);
            }
        }
    } catch (pj::Error &err) {
        qCCritical(lcSIPAccount) << "failed to create transport:" << err.info(false);
        Q_EMIT initialized(false);
        return;
    }

    // NAT config
    if (m_settings.contains("sipStunUse")) {
        m_accountConfig.natConfig.sipStunUse = m_settings.value("sipStunUse").toBool()
                ? PJSUA_STUN_RETRY_ON_FAILURE
                : PJSUA_STUN_USE_DISABLED;
    }
    if (m_settings.contains("mediaStunUse")) {
        m_accountConfig.natConfig.mediaStunUse = m_settings.value("mediaStunUse").toBool()
                ? PJSUA_STUN_RETRY_ON_FAILURE
                : PJSUA_STUN_USE_DISABLED;
    }
    if (m_settings.contains("sipUpnpUse")) {
        m_accountConfig.natConfig.sipUpnpUse = m_settings.value("sipUpnpUse").toBool()
                ? PJSUA_UPNP_USE_DEFAULT
                : PJSUA_UPNP_USE_DISABLED;
    }
    if (m_settings.contains("nat64Opt")) {
        m_accountConfig.natConfig.nat64Opt =
                m_settings.value("nat64Opt").toBool() ? PJSUA_NAT64_ENABLED : PJSUA_NAT64_DISABLED;
    }

    unsigned contactRewriteUse = m_settings.value("contactRewriteUse", 0).toUInt(&ok);
    if (ok && contactRewriteUse < 3) {
        m_accountConfig.natConfig.contactRewriteUse = contactRewriteUse;
    } else {
        qCCritical(lcSIPAccount) << "invalid value for 'contactRewriteUse' [0,1,2]:"
                                 << contactRewriteUse;
        Q_EMIT initialized(false);
        return;
    }
    if (m_settings.contains("contactUseSrcPort")) {
        m_accountConfig.natConfig.contactUseSrcPort =
                m_settings.value("contactUseSrcPort").toBool();
    }
    QString contactRewriteMethod = m_settings.value("contactRewriteMethod").toString();
    if (!contactRewriteMethod.isEmpty()) {
        if (contactRewriteMethod == "always-update") {
            m_accountConfig.natConfig.contactRewriteMethod = PJSUA_CONTACT_REWRITE_ALWAYS_UPDATE;
        } else if (contactRewriteMethod == "no-unreg") {
            m_accountConfig.natConfig.contactRewriteMethod = PJSUA_CONTACT_REWRITE_NO_UNREG;
        } else {
            qCCritical(lcSIPAccount)
                    << "invalid value for 'contactRewriteMethod' [always-update,no-unreg]:"
                    << contactRewriteMethod;
            Q_EMIT initialized(false);
            return;
        }
    }

    if (m_settings.contains("iceEnabled")) {
        m_accountConfig.natConfig.iceEnabled = m_settings.value("iceEnabled").toBool();
    }
    if (m_settings.contains("iceAggressiveNomination")) {
        m_accountConfig.natConfig.iceAggressiveNomination =
                m_settings.value("iceAggressiveNomination").toBool();
    }
    if (m_settings.contains("iceNoRtcp")) {
        m_accountConfig.natConfig.iceNoRtcp = m_settings.value("iceNoRtcp").toBool();
    }
    if (m_settings.contains("iceAlwaysUpdate")) {
        m_accountConfig.natConfig.iceAlwaysUpdate = m_settings.value("iceAlwaysUpdate").toBool();
    }

    if (m_settings.contains("turnEnabled")) {
        m_accountConfig.natConfig.turnEnabled = m_settings.value("turnEnabled").toBool();
    }

    if (m_settings.contains("viaRewriteUse")) {
        m_accountConfig.natConfig.viaRewriteUse = m_settings.value("viaRewriteUse").toBool();
    }
    if (m_settings.contains("sdpNatRewriteUse")) {
        m_accountConfig.natConfig.sdpNatRewriteUse = m_settings.value("sdpNatRewriteUse").toBool();
    }

    if (m_settings.contains("sipOutboundUse")) {
        m_accountConfig.natConfig.sipOutboundUse = m_settings.value("sipOutboundUse").toBool();
    }
    if (m_settings.contains("sipOutboundInstanceId")) {
        m_accountConfig.natConfig.sipOutboundInstanceId =
                m_settings.value("sipOutboundInstanceId").toString().toStdString();
    }
    if (m_settings.contains("sipOutboundRegId")) {
        m_accountConfig.natConfig.sipOutboundRegId =
                m_settings.value("sipOutboundRegId").toString().toStdString();
    }

    m_accountConfig.natConfig.contactRewriteUse = 0;

    m_settings.endGroup();

    // Authentication setup
    QString auth = m_settings.value(m_account + "/auth").toString();
    if (!auth.isEmpty()) {

        if (!m_settings.childGroups().contains(auth)) {
            qCCritical(lcSIPAccount) << "'auth' contains undefined reference to:" << auth;
            Q_EMIT initialized(false);
            return;
        }

        m_settings.beginGroup(auth);
        QString scheme = m_settings.value("scheme", "Digest").toString();
        QString username = m_settings.value("username").toString();
        QString realm = m_settings.value("realm", "*").toString();
        QString data = m_settings.value("data").toString();
        QString dataType = m_settings.value("type", "plain").toString();
        int dataTypeValue = PJSIP_CRED_DATA_PLAIN_PASSWD;
        if (dataType == "plain") {
            dataTypeValue = PJSIP_CRED_DATA_PLAIN_PASSWD;
        } else if (dataType == "digest") {
            dataTypeValue = PJSIP_CRED_DATA_DIGEST;
        } else {
            qCCritical(lcSIPAccount) << "invalid value for 'type' [plain, digest]:" << dataType;
            Q_EMIT initialized(false);
            return;
        }

        m_settings.endGroup();

        // Allow plain text password for debugging cases
        if (data.isEmpty()) {
            auto &cds = Credentials::instance();
            cds.get(auth + "/secret",
                    [this, auth, scheme, realm, username, dataTypeValue](bool error,
                                                                         const QString &data) {
                        if (error) {
                            qCCritical(lcSIPAccount) << "authentification failed:" << data;
                            Q_EMIT initialized(false);
                            return;
                        }

                        if (data.isEmpty()) {
                            qCWarning(lcSIPAccount)
                                    << "no password available for auth group" << auth;
                        }

                        pj::AuthCredInfo cred(scheme.toStdString(), realm.toStdString(),
                                              username.toStdString(), dataTypeValue,
                                              data.toStdString());

                        m_accountConfig.sipConfig.authCreds.push_back(cred);

                        finalizeInitialization();
                    });

            return;
        }

        pj::AuthCredInfo cred(scheme.toStdString(), realm.toStdString(), username.toStdString(),
                              dataTypeValue, data.toStdString());

        m_accountConfig.sipConfig.authCreds.push_back(cred);
    }

    finalizeInitialization();
}

void SIPAccount::finalizeInitialization()
{
    try {
        create(m_accountConfig);
    } catch (pj::Error &err) {
        qCCritical(lcSIPAccount) << "failed to create" << m_account << ":" << err.info(false);
        ErrorBus::instance().addFatalError(
                tr("Failed to create %1: %2")
                        .arg(m_account, QString::fromLocal8Bit(err.info(false))));
        Q_EMIT initialized(false);
        return;
    }

    connect(&NetworkHelper::instance(), &NetworkHelper::connectivityChanged, this, []() {
        if (NetworkHelper::instance().hasConnectivity()
            && !SIPCallManager::instance().hasActiveCalls()) {
            try {
                pj::Endpoint::instance().handleIpChange(pj::IpChangeParam());
            } catch (pj::Error &err) {
                qCCritical(lcSIPAccount) << "Ignoring pjsip error on handle ip change:"
                                         << QString::fromLocal8Bit(err.info(false));
            }
        }
    });

    Q_EMIT initialized(true);
}

QString SIPAccount::call(const QString &number, const QString &contactId,
                         const QString &preferredIdentity, bool silent)
{
    QString sipUrl = toSipUri(number);

    qCInfo(lcSIPAccount) << "DIAL" << sipUrl;

    // Create call
    SIPCall *call = new SIPCall(this, PJSUA_INVALID_ID, contactId, silent);

    pj::CallOpParam prm(true);
    prm.opt.audioCount = 1;
    prm.opt.videoCount = 0;

    generatePreferredIdentityHeader(number, preferredIdentity, prm);

    try {
        call->call(sipUrl, prm);
        m_calls.push_back(call);
        return call->uuid();
    } catch (pj::Error &err) {
        qCCritical(lcSIPAccount) << "Dialing failed:" << err.info();
        delete call;
    }

    return "";
}

long SIPAccount::sendMessage(const QString &recipient, const QString &message,
                             const QString &mimeType)
{
    long id = SIPAccount::runningMessageIndex++;
    QString sipUrl = toSipUri(recipient);
    SIPBuddy *foundBuddy = nullptr;

    for (const auto buddy : std::as_const(m_buddies)) {
        if (buddy->uri() == sipUrl) {
            foundBuddy = buddy;
        }
    }

    if (foundBuddy) {
        pj::SendInstantMessageParam msg;
        msg.contentType = mimeType.toStdString();
        msg.content = message.toStdString();
        msg.userData = (void *)id;

        try {
            foundBuddy->sendInstantMessage(msg);
        } catch (pj::Error &err) {
            qCWarning(lcSIPAccount) << "failed to send message:" << err.info();
        }
    } else {
        qCWarning(lcSIPAccount) << "failed to send message: unknown SIP buddy";
    }

    return id;
}

void SIPAccount::onInstantMessageStatus(pj::OnInstantMessageStatusParam &prm)
{
    qCDebug(lcSIPAccount) << "sent message to " << prm.toUri << ", status: " << prm.code
                          << prm.reason << (long)prm.userData;
}

void SIPAccount::onInstantMessage(pj::OnInstantMessageParam &prm)
{
    qCDebug(lcSIPAccount) << "received message from " << prm.fromUri << ":" << prm.msgBody;
    Q_EMIT messageReceived(prm.fromUri.c_str(), prm.msgBody.c_str(), prm.contentType.c_str());
}

SIPBuddyState::STATUS SIPAccount::buddyStatus(const QString &var)
{
    QString sipUrl = toSipUri(var);

    // Do we have such a buddy already?
    for (auto buddy : std::as_const(m_buddies)) {
        if (buddy->uri() == sipUrl) {
            return buddy->status();
        }
    }

    // We don't have a buddy yet - try to subscribe
    auto buddy = new SIPBuddy(this, sipUrl);
    qCInfo(lcSIPAccount) << "subscribing to buddy" << buddy->uri();

    if (buddy->initialize()) {
        m_buddies.push_back(buddy);
        connect(buddy, &SIPBuddy::destroyed, this, [buddy, this]() {
            qCCritical(lcSIPAccount) << "removing buddy" << buddy->uri();
            m_buddies.removeAll(buddy);
        });
    } else {
        buddy->deleteLater();
    }

    return SIPBuddyState::STATUS::UNKNOWN;
}

QString SIPAccount::toSipUri(const QString &var) const
{
    static QRegularExpression sugarStripper("[\\s()/\\.-]");
    QString sipUrl;

    if (PhoneNumberUtil::isSipUri(var)) {
        sipUrl = var;
    } else {
        QString number = var;

        // Remove whitespace, braces, dashes, slashes and dots
        number = number.remove(sugarStripper);

        // Split at the first ","
        number = number.section(',', 0, 0);

        sipUrl = QString("sip:%1@%2").arg(number, m_domain);

        if (!PhoneNumberUtil::isSipUri(sipUrl)) {
            qCCritical(lcSIPAccount) << "invalid SIP URI passed:" << sipUrl;
            sipUrl = "";
        }
    }

    return sipUrl;
}

void SIPAccount::generatePreferredIdentityHeader(const QString &var,
                                                 const QString &preferredIdentity,
                                                 pj::CallOpParam &prm)
{
    // Convert var to number
    QString number = var;
    if (PhoneNumberUtil::isSipUri(number)) {
        ContactInfo info = PhoneNumberUtil::instance().contactInfoBySipUrl(var);
        number = info.phoneNumber;
    }

    // Feed developer mode "invite headers" section
    m_settings.beginGroup("invite headers");

    auto headers = m_settings.allKeys();
    for (auto &header : std::as_const(headers)) {
        pj::SipHeader h;
        h.hName = header.toStdString();
        h.hValue = m_settings.value(header).toString().toStdString();
        prm.txOption.headers.push_back(h);
        qCWarning(lcSIPAccount) << "adding development header" << header << ":"
                                << m_settings.value(header);
    }

    m_settings.endGroup();

    // Don't do anything special for "default" mode
    if (preferredIdentity == "default") {
        return;
    }

    // Automatic mode, check all preferred identities, if a rewrite is desired
    auto identities = SIPManager::instance().enrolledPreferredIdentities();

    if (preferredIdentity == "auto") {
        for (auto identity : std::as_const(identities)) {
            if (!identity->enabled() || !identity->automatic()) {
                continue;
            }

            if (identity->prefix().isEmpty() || number.startsWith(identity->prefix())) {
                pj::SipHeader h;
                h.hName = "P-Preferred-Identity";
                h.hValue = "tel:" + identity->identity().toStdString();
                prm.txOption.headers.push_back(h);

                qCInfo(lcSIPAccount) << "setting preferred identity for call to " << number
                                     << "to be" << identity->identity();
                break;
            }
        }

        return;
    }

    // Neither "default", nor "auto" -> try to find the desired identity
    for (auto identity : std::as_const(identities)) {
        if (identity->id() == preferredIdentity) {
            pj::SipHeader h;
            h.hName = "P-Preferred-Identity";
            h.hValue = "tel:" + identity->identity().toStdString();
            prm.txOption.headers.push_back(h);

            qCInfo(lcSIPAccount) << "overriding preferred identity for call to " << number
                                 << "to be" << identity->identity();
        }
    }
}

bool SIPAccount::hasAllowGrant(const QString &header, const QString &grant) const
{

    // return header.contains("Allow:");
    static const QRegularExpression regex("Allow: (?<grants>.*?)(\\r\\n?|\\n)");

    const auto matchResult = regex.match(header);
    if (matchResult.hasMatch()) {
        const auto grants = matchResult.captured("grants").split(", ");
        return grants.contains(grant);
    }

    return false;
}

SIPCall *SIPAccount::getCallById(const int callId)
{
    for (auto call : std::as_const(m_calls)) {
        if (call->getId() == callId) {
            return call;
        }
    }

    return nullptr;
}

void SIPAccount::hangup(const int callId)
{
    SIPCall *foundCall = getCallById(callId);

    if (foundCall) {
        pj::CallOpParam prm;
        prm.statusCode = PJSIP_SC_OK;

        try {
            foundCall->hangup(prm);
        } catch (pj::Error &err) {
            qCCritical(lcSIPAccount) << "failed to hangup call" << ":" << err.info(false);
        }
    }
}

void SIPAccount::hold(const int callId)
{
    SIPCall *foundCall = getCallById(callId);

    if (foundCall) {
        foundCall->hold();
    }
}

void SIPAccount::unhold(const int callId)
{
    SIPCall *foundCall = getCallById(callId);

    if (foundCall) {
        foundCall->unhold();
    }
}

void SIPAccount::removeCall(const int callId)
{
    SIPCall *foundCall = getCallById(callId);
    removeCall(foundCall);
}

void SIPAccount::removeCall(SIPCall *call)
{
    if (call) {
        m_calls.removeAll(call);
        delete call;
    }
}

uint SIPAccount::retryInterval() const
{
    return m_accountConfig.regConfig.retryIntervalSec;
}

void SIPAccount::onIncomingCall(pj::OnIncomingCallParam &iprm)
{
    SIPCall *call = new SIPCall(this, iprm.callId);
    call->setIncoming(true);
    pj::CallInfo ci = call->getInfo();

    qCInfo(lcSIPAccount) << "Incoming Call:" << ci.remoteUri << " [" << ci.stateText << "]";

    m_calls.push_back(call);

    Q_EMIT SIPCallManager::instance().incomingCall(call);
}

void SIPAccount::onRegState(pj::OnRegStateParam &prm)
{
    pj::AccountInfo ai = getInfo();
    qCInfo(lcSIPAccount).noquote().nospace()
            << "Account " << m_account
            << (ai.regIsActive ? " registered: (code=" : " unregister: (code=") << prm.code << ")";

    if (m_isRegistered != ai.regIsActive) {
        m_isRegistered = ai.regIsActive;
        Q_EMIT isRegisteredChanged();
    }

    if (prm.code == PJSIP_SC_UNAUTHORIZED) {
        Q_EMIT authorizationFailed();
    }

    if (!m_useInstantMessagingWithoutCheck && m_isRegistered && m_shallNegotiateCapabilities) {
        pj::SipHeaderVector headers;
        pj::SipHeader contactHeader;
        contactHeader.hName = "Contact";
        contactHeader.hValue = "<"
                + addTransport(QString::fromStdString(m_accountConfig.idUri)).toStdString() + ">";
        headers.push_back(contactHeader);

        pj::SipTxOption opt;
        opt.targetUri = m_accountConfig.regConfig.registrarUri;
        opt.headers = headers;

        m_optionsRequestUuid = QUuid::createUuid().toByteArray();

        pj::SendRequestParam prm;
        prm.method = "OPTIONS";
        prm.txOption = opt;
        prm.userData = m_optionsRequestUuid.data();

        sendRequest(prm);
    }
}

void SIPAccount::onSendRequest(pj::OnSendRequestParam &prm)
{
    const QByteArray uuid(static_cast<char *>(prm.userData));

    if (uuid == m_optionsRequestUuid) {
        m_optionsRequestUuid.clear();
        const auto header = QString::fromStdString(prm.e.body.tsxState.src.rdata.wholeMsg);
        m_isInstantMessagingAllowed = hasAllowGrant(header, "MESSAGE");
    }
}

bool SIPAccount::isInstantMessagingAllowed() const
{
    if (m_useInstantMessagingWithoutCheck) {
        return true;
    }
    return m_shallNegotiateCapabilities && m_isInstantMessagingAllowed;
}

QString SIPAccount::addTransport(const QString &in)
{
    if (m_transportType == TRANSPORT_TYPE::TLS) {
        return in + ";transport=tls";
    } else if (m_transportType == TRANSPORT_TYPE::TCP) {
        return in + ";transport=tcp";
    } else if (m_transportType == TRANSPORT_TYPE::UDP) {
        return in + ";transport=udp";
    }

    return in;
}

void SIPAccount::setCredentials(const QString &password)
{
    QString authGroup = m_settings.value(m_account + "/auth").toString();
    if (authGroup.isEmpty()) {
        qCCritical(lcSIPAccount) << "no auth group specified - bailing out";
        return;
    }

    // Update password in SIP config
    if (m_accountConfig.sipConfig.authCreds.size()) {
        pj::AuthCredInfo &info = m_accountConfig.sipConfig.authCreds.front();
        info.data = password.toStdString();
        modify(m_accountConfig);
    }

    // Update storage
    Credentials::instance().set(
            authGroup + "/secret", password, [authGroup](bool error, const QString &data) {
                if (error) {
                    qCCritical(lcSIPAccount) << "failed to set credentials:" << data;
                }
            });
}

SIPAccount::~SIPAccount()
{
    qDeleteAll(m_calls);
    m_calls.clear();
}
