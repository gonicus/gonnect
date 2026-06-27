#include <QQmlEngine>
#include <QQmlContext>
#include <QLoggingCategory>
#include <QTimer>
#include <QRegularExpression>

#include "Application.h"
#include "SIPEventLoop.h"
#include "SIPManager.h"
#include "SIPMediaConfig.h"
#include "SIPUserAgentConfig.h"
#include "SIPAccountManager.h"
#include "NetworkHelper.h"
#include "AudioManager.h"

#include <pjsua-lib/pjsua.h>
#include <pjsip/sip_endpoint.h>
#include <pjlib-util/resolver.h>
#include "VideoManager.h"
#include "TogglerManager.h"

Q_LOGGING_CATEGORY(lcSIPManager, "gonnect.sip.manager")
Q_LOGGING_CATEGORY(lcPJSIP, "gonnect.pjsip")

using std::chrono::seconds;
using namespace std::chrono_literals;

void SIPLogWriter::write(const pj::LogEntry &entry)
{
    // Remove first / last "
    auto msg = QString::fromStdString(entry.msg);

    switch (entry.level) {
    case 1: // only errors
        qCWarning(lcPJSIP).noquote() << msg;
        break;

    case 2: // info
    case 3:
        qCInfo(lcPJSIP).noquote() << msg;
        break;

    case 4: // debug
    case 5:
    case 6:
        qCDebug(lcPJSIP).noquote() << msg;
        break;
    }
}

SIPManager::SIPManager(QObject *parent) : QObject(parent)
{
    m_settings = std::make_unique<AppSettings>();
}

void SIPManager::initialize()
{
    qCDebug(lcSIPManager) << "initializing";
    pj_log_set_level(0);

    ReadOnlyConfdSettings globalSettings;
    if (globalSettings.value("sip/compactHeader", false).toBool()) {
        qCDebug(lcSIPManager) << "enabling compact headers";
        pjsip_cfg()->endpt.use_compact_form = PJ_TRUE;
    }
    if (globalSettings.value("sip/noTcpSwitch", false).toBool()) {
        qCDebug(lcSIPManager) << "disabling TCP switch";
        pjsip_cfg()->endpt.disable_tcp_switch = PJ_TRUE;
    }

    try {
        m_ep.libCreate();
    } catch (pj::Error &err) {
        qCFatal(lcSIPManager) << "failed to initialize SIP library: " << err.info();
    }

    // Initial configuration
    pj::EpConfig epConfig;

    auto app = qobject_cast<Application *>(Application::instance());
    if (app->isDebugRun()) {
        epConfig.logConfig.level = 6;
    } else {
        ReadOnlyConfdSettings settings;
        epConfig.logConfig.level = settings.value("logging/level", 1).toUInt();
    }

    if (epConfig.logConfig.level >= 4) {
        QLoggingCategory::setFilterRules(QStringLiteral("gonnect.*=true\n"
                                                        "*.warning=true\n"
                                                        "*.critical=true"));
    } else if (epConfig.logConfig.level >= 2) {
        QLoggingCategory::setFilterRules(QStringLiteral("gonnect.info=true\n"
                                                        "*.warning=true\n"
                                                        "*.critical=true"));
    }

    epConfig.uaConfig.mwiUnsolicitedEnabled = true;

    m_mediaConfig = new SIPMediaConfig(this);
    m_mediaConfig->applyConfig(epConfig);

    m_uaConfig = new SIPUserAgentConfig(this);
    m_uaConfig->applyConfig(epConfig);

    // Setup log writer
    m_logWriter = std::make_unique<SIPLogWriter>();

    pj::LogConfig *log_cfg = &epConfig.logConfig;
    log_cfg->writer = m_logWriter.get();
    log_cfg->decor = log_cfg->decor
            & ~(::pj_log_decoration::PJ_LOG_HAS_CR | ::pj_log_decoration::PJ_LOG_HAS_NEWLINE
                | ::pj_log_decoration::PJ_LOG_HAS_TIME | ::pj_log_decoration::PJ_LOG_HAS_MICRO_SEC);

    try {
        m_ep.libInit(epConfig);
    } catch (pj::Error &err) {
        qCFatal(lcSIPManager) << "failed to initialize SIP library: " << err.info();
    }

    // Set codec preference
    setPreferredCodecs();

    m_ep.libStart();

    configureDnsResolver();

    // Load Accounts + Transports
    auto &sam = SIPAccountManager::instance();
    connect(&sam, &SIPAccountManager::accountsChanged, this, [this]() {
        for (const QString &var : std::as_const(m_buddyStateQueue)) {
            buddyStatus(var);
        }
        m_buddyStateQueue.clear();
    });
    sam.initialize();

    TogglerManager::instance().initialize();

    // Create event loop object
    m_ev = new SIPEventLoop(this);

    // Get audio on the road
    AudioManager::instance().initialize();
    VideoManager::instance().updateDevices();

    m_defaultPreferredIdentity = m_settings->value("generic/preferredIdentity", "auto").toString();
    Q_EMIT defaultPreferredIdentityChanged();
    initializePreferredIdentities();

    if (!isConfigured()) {
        Q_EMIT notConfigured();
    }

    // Install network recovery timer
    m_networkRecoveryTimer.setSingleShot(true);
    m_networkRecoveryTimer.setInterval(1s);
    connect(&m_networkRecoveryTimer, &QTimer::timeout, this, &SIPManager::recoverFromNetworkChange);

    // Configure registration recovery watchdog
    m_registrationCheckTimer.setSingleShot(true);
    connect(&m_registrationCheckTimer, &QTimer::timeout, this, &SIPManager::checkRecovery);

    m_initialized = true;
}

void SIPManager::setPreferredCodecs()
{
    const QList<int> codecPriorities = { PJMEDIA_CODEC_PRIO_HIGHEST, PJMEDIA_CODEC_PRIO_NEXT_HIGHER,
                                         PJMEDIA_CODEC_PRIO_NORMAL, PJMEDIA_CODEC_PRIO_LOWEST };

    ReadOnlyConfdSettings globalSettings;
    const QList<QString> preferredCodecs =
            globalSettings.value("sip/preferredCodecs", "").toStringList();
    if (preferredCodecs.empty()) {
        return;
    }

    // Check if there's valid codecs in our preference list
    QStringList registeredCodecs;
    try {
        const auto &codecs = m_ep.codecEnum2();
        for (const auto &c : std::as_const(codecs)) {
            registeredCodecs << QString::fromStdString(c.codecId);
        }
    } catch (const pj::Error &err) {
        qCWarning(lcSIPManager)
                << "failed to enumerate codecs - skipping preferred codec selection:"
                << QString::fromStdString(err.info());
        return;
    }

    QList<std::pair<QString, int>> resolvedCodecs;
    for (int i = 0; i < preferredCodecs.count(); i++) {
        const QString &preferredCodec = preferredCodecs.at(i);
        const int priority = codecPriorities.at(qMin(i, codecPriorities.count() - 1));
        const QString prefix = preferredCodec + "/";

        bool matched = false;
        for (const QString &registered : std::as_const(registeredCodecs)) {
            if (registered.compare(preferredCodec, Qt::CaseInsensitive) == 0
                || registered.startsWith(prefix, Qt::CaseInsensitive)) {
                resolvedCodecs.append({ registered, priority });
                matched = true;
            }
        }

        if (!matched) {
            qCWarning(lcSIPManager) << "ignoring unknown preferred codec" << preferredCodec
                                    << "- not among registered codecs" << registeredCodecs;
        }
    }

    if (resolvedCodecs.empty()) {
        qCWarning(lcSIPManager)
                << "no valid preferred codec found - skipping preferred codec selection";
        return;
    }

    // Disable all codecs
    for (const QString &registered : std::as_const(registeredCodecs)) {
        try {
            m_ep.codecSetPriority(registered.toStdString(), PJMEDIA_CODEC_PRIO_DISABLED);
        } catch (const pj::Error &err) {
            qCWarning(lcSIPManager) << "failed to disable codec" << registered << ":"
                                    << QString::fromStdString(err.info());
        }
    }

    // Only enable/use config codecs
    for (const auto &[codecId, priority] : std::as_const(resolvedCodecs)) {
        try {
            m_ep.codecSetPriority(codecId.toStdString(), priority);
            qCDebug(lcSIPManager) << "using codec" << codecId << ", with priority" << priority;
        } catch (const pj::Error &err) {
            qCWarning(lcSIPManager) << "failed to enable codec" << codecId << ":"
                                    << QString::fromStdString(err.info());
        }
    }
}

bool SIPManager::isConfigured() const
{
    return SIPAccountManager::instance().hasConfiguration();
}

void SIPManager::initializePreferredIdentities()
{
    static const QRegularExpression identityRegex("^preferred_identity_[0-9]+$");
    auto sections = m_settings->childGroups();

    m_preferredIdentities.clear();

    for (auto &section : std::as_const(sections)) {
        auto match = identityRegex.match(section);
        if (match.hasMatch()) {
            auto identity = new PreferredIdentity(this, section);
            if (identity->isValid()) {
                m_preferredIdentities.push_back(identity);
                connect(identity, &PreferredIdentity::prefixChanged, this,
                        &SIPManager::updatePreferredIdentities);
            } else {
                qCCritical(lcSIPManager)
                        << "invalid preferred identity" << section << "has been skipped";
                delete identity;
            }
        }
    }

    updatePreferredIdentities();
}

PreferredIdentity *SIPManager::addEmptyPreferredIdentity()
{

    // Finde highest existing identity section id
    uint maxId = 0;
    for (const auto identity : std::as_const(m_preferredIdentities)) {
        const auto currId = identity->id().split(QChar('_')).last().toUInt();
        if (currId > maxId) {
            maxId = currId;
        }
    }

    // Create new identity object
    auto identity =
            new PreferredIdentity(this, QString::asprintf("preferred_identity_%u", maxId + 1));
    identity->setDisplayName(tr("New Identity"));
    identity->setIdentity("+49221123456789");
    identity->setPrefix("");
    identity->setEnabled(true);
    identity->setAutomatic(true);

    m_preferredIdentities.push_back(identity);
    updatePreferredIdentities();

    return identity;
}

void SIPManager::removePreferredIdentity(PreferredIdentity *preferredIdentity)
{
    Q_CHECK_PTR(preferredIdentity);

    m_settings->remove(preferredIdentity->id());
    m_preferredIdentities.removeOne(preferredIdentity);
    preferredIdentity->deleteLater();

    updatePreferredIdentities();
}

SIPBuddyState::STATUS SIPManager::buddyStatus(const QString &var)
{
    auto accounts = SIPAccountManager::instance().accounts();

    // Hard code to the one and only account for now
    if (accounts.count()) {
        return accounts.first()->buddyStatus(var);
    } else {
        // Save number/url for processing after account has been initialized
        m_buddyStateQueue.insert(var);
    }

    return SIPBuddyState::STATUS::UNKNOWN;
}

void SIPManager::updatePreferredIdentities()
{
    // Build up an enrolled identities list for SIPAccount lookup
    qDeleteAll(m_enrolledPreferredIdentities);
    m_enrolledPreferredIdentities.clear();
    for (auto identity : std::as_const(m_preferredIdentities)) {
        auto prefixes = identity->prefix().split(",");
        for (auto &prefix : std::as_const(prefixes)) {
            auto npi = new PreferredIdentity(*identity);
            npi->setParent(this);
            npi->setPrefix(prefix.trimmed());
            m_enrolledPreferredIdentities.push_back(npi);
        }
    }

    // Sort identities by prefix length, descending
    std::sort(m_enrolledPreferredIdentities.begin(), m_enrolledPreferredIdentities.end(),
              [](const PreferredIdentity *a, const PreferredIdentity *b) {
                  return a->prefix().length() > b->prefix().length();
              });

    // Check preferred identity and reset it to a valid value if not there
    if (m_defaultPreferredIdentity != "auto" && m_defaultPreferredIdentity != "default") {
        bool found = false;
        for (auto identity : std::as_const(m_preferredIdentities)) {
            if (identity->id() == m_defaultPreferredIdentity) {
                found = true;
                break;
            }
        }

        if (!found) {
            qCCritical(lcSIPManager)
                    << "invalid preferred identity for account - reverting to 'auto'";
            m_defaultPreferredIdentity = "auto";
            Q_EMIT defaultPreferredIdentityChanged();
        }
    }

    Q_EMIT preferredIdentitiesChanged();
}

SIPBuddy *SIPManager::getBuddy(const QString &var)
{
    auto accounts = SIPAccountManager::instance().accounts();
    if (!accounts.count()) {
        qCDebug(lcSIPManager) << "could not retrieve accounts";
        return nullptr;
    }

    // There is only one account as of now
    SIPAccount *account = accounts.first();
    QList<SIPBuddy *> buddies = account->buddies();
    QString uri = account->toSipUri(var);

    for (SIPBuddy *buddy : std::as_const(buddies)) {
        if (buddy->uri() == uri) {
            return buddy;
        }
    }

    qCDebug(lcSIPManager) << "could not find the corresponding buddy";
    return nullptr;
}

void SIPManager::suspend()
{
    qCDebug(lcSIPManager) << "suspending SIP";
    m_suspended = true;

    pj::CallOpParam prm;
    prm.statusCode = PJSIP_SC_SERVICE_UNAVAILABLE;

    // Hang up all calls
    auto calls = SIPCallManager::instance().calls();
    for (auto call : std::as_const(calls)) {
        call->hangup(prm);
    }

    // Unregister account(s)
    auto accounts = SIPAccountManager::instance().accounts();
    for (auto account : std::as_const(accounts)) {
        account->deactivateTransports();
    }

    // Shutdown transports
    pj::IpChangeParam param;
    param.shutdownTransport = true;
    param.restartListener = false;
    pj::Endpoint::instance().handleIpChange(param);
}

void SIPManager::resume()
{
    if (!m_suspended) {
        return;
    }

    m_suspended = false;

    // Since resume may be called in more network changed cases, only
    // do this when we have no active calls going.
    if (!SIPCallManager::instance().hasActiveCalls()) {
        qCDebug(lcSIPManager) << "resuming SIP";

        // Activate transports again
        auto accounts = SIPAccountManager::instance().accounts();
        for (auto account : std::as_const(accounts)) {
            account->setAfterResume();
            account->setRegistration(false);
            account->activateTransports();
        }

        try {
            pj::Endpoint::instance().handleIpChange(pj::IpChangeParam());
        } catch (pj::Error &err) {
            qCCritical(lcSIPManager)
                    << "error handling IP change:" << QString::fromLocal8Bit(err.info(false));
        }

        // Re-activate account registration
        for (auto account : std::as_const(accounts)) {
            account->setRegistration(true);
        }
    }
}

void SIPManager::handleNetworkChanged()
{
    if (!m_initialized) {
        return;
    }

    if (m_suspended) {
        resume();
        return;
    }

    qCDebug(lcSIPManager) << "network changed - scheduling SIP recovery";
    m_networkRecoveryAttempts = 0;
    m_registrationCheckTimer.stop();
    m_networkRecoveryTimer.start(1s);
}

void SIPManager::recoverFromNetworkChange()
{
    if (!m_initialized || m_suspended) {
        return;
    }

    m_registrationCheckTimer.stop();

    qCDebug(lcSIPManager) << "network settled - recovering SIP (attempt"
                          << (m_networkRecoveryAttempts + 1) << "of" << s_maxNetworkRecoveryAttempts
                          << ")";

    auto accounts = SIPAccountManager::instance().accounts();

    for (auto account : std::as_const(accounts)) {
        account->setAfterResume();
    }

    resetDnsResolver();

    try {
        pj::Endpoint::instance().handleIpChange(pj::IpChangeParam());
    } catch (pj::Error &err) {
        if (++m_networkRecoveryAttempts < s_maxNetworkRecoveryAttempts) {
            const auto delay = std::min(seconds(1 << m_networkRecoveryAttempts), 8s);
            qCWarning(lcSIPManager).nospace()
                    << "IP change handling failed, retrying in " << delay.count()
                    << "s: " << QString::fromLocal8Bit(err.info(false));
            m_networkRecoveryTimer.start(delay);
        } else {
            qCCritical(lcSIPManager) << "giving up SIP recovery after" << m_networkRecoveryAttempts
                                     << "attempts:" << QString::fromLocal8Bit(err.info(false));
        }
        return;
    }

    // Check if registration worked - if DNS worked, but the route to the SIP
    // server was not yet established, we're running into a case where pijsip
    // never tries to register again.
    m_registrationCheckTimer.start(20s);
}

void SIPManager::checkRecovery()
{
    if (!m_initialized || m_suspended) {
        return;
    }

    const auto accounts = SIPAccountManager::instance().accounts();

    if (accounts.isEmpty()) {
        m_networkRecoveryAttempts = 0;
        return;
    }

    bool allRegistered = true;
    for (auto account : std::as_const(accounts)) {
        if (!account->isRegistered()) {
            allRegistered = false;
            break;
        }
    }

    if (allRegistered) {
        qCDebug(lcSIPManager) << "SIP recovered successfully";
        m_networkRecoveryAttempts = 0;
        return;
    }

    if (++m_networkRecoveryAttempts < s_maxNetworkRecoveryAttempts) {
        const auto delay = std::min(seconds(1 << m_networkRecoveryAttempts), 8s);
        qCWarning(lcSIPManager).nospace()
                << "SIP recovery did not register - retrying in " << delay.count() << "seconds";
        m_networkRecoveryTimer.start(delay);
    } else {
        qCCritical(lcSIPManager) << "giving up SIP recovery after" << m_networkRecoveryAttempts
                                 << "attempts";
    }
}

void SIPManager::configureDnsResolver()
{
    // Disable pjsip's DNS response cache - the OS already has a cache
    pjsip_endpoint *endpt = pjsua_get_pjsip_endpt();
    if (!endpt) {
        return;
    }
    pj_dns_resolver *resolver = pjsip_endpt_get_resolver(endpt);
    if (!resolver) {
        return;
    }

    pj_dns_settings settings;
    pj_dns_resolver_get_settings(resolver, &settings);
    settings.cache_max_ttl = 0;
    const pj_status_t status = pj_dns_resolver_set_settings(resolver, &settings);
    if (status != PJ_SUCCESS) {
        char errbuf[PJ_ERR_MSG_SIZE];
        pj_strerror(status, errbuf, sizeof(errbuf));
        qCWarning(lcSIPManager) << "failed to disable DNS resolver cache:" << errbuf;
    }
}

void SIPManager::resetDnsResolver()
{
    pjsip_endpoint *endpt = pjsua_get_pjsip_endpt();
    if (!endpt) {
        return;
    }

    pj_dns_resolver *resolver = pjsip_endpt_get_resolver(endpt);
    if (!resolver) {
        // No DNS resolver configured
        return;
    }

    ReadOnlyConfdSettings settings;
    const QStringList nameservers =
            settings.value("ua/nameservers", NetworkHelper::instance().nameservers())
                    .toStringList();

    std::vector<std::string> storage;
    storage.reserve(nameservers.size());
    for (const auto &ns : nameservers) {
        if (!ns.isEmpty()) {
            storage.push_back(ns.toStdString());
        }
    }

    if (storage.empty()) {
        return;
    }

    std::vector<pj_str_t> servers;
    servers.reserve(storage.size());
    for (auto &s : storage) {
        servers.push_back(pj_str(const_cast<char *>(s.c_str())));
    }

    const pj_status_t status = pj_dns_resolver_set_ns(
            resolver, static_cast<unsigned>(servers.size()), servers.data(), nullptr);
    if (status != PJ_SUCCESS) {
        char errbuf[PJ_ERR_MSG_SIZE];
        pj_strerror(status, errbuf, sizeof(errbuf));
        qCWarning(lcSIPManager) << "failed to reset DNS resolver nameservers:" << errbuf;
    }
}

void SIPManager::shutdown()
{
    qCDebug(lcSIPManager) << "shutting down";

    m_ep.hangupAllCalls();

    // Release ownership of the log writer to pjsua2, which will delete it
    // during libDestroy(). Without this, libDestroy() logs internally and
    // calls into the already-deleted writer, or double-deletes it.
    m_logWriter.release();

    try {
        delete m_ev;
        m_ep.libDestroy();
    } catch (pj::Error &err) {
        qCWarning(lcSIPManager) << "error shutting down pjsip:" << err.info();
    }

    m_settings.reset();

    qDeleteAll(m_enrolledPreferredIdentities);
    m_enrolledPreferredIdentities.clear();
}

void SIPManager::setDefaultPreferredIdentity(const QString &value)
{
    if (m_defaultPreferredIdentity != value) {
        m_settings->setValue("generic/preferredIdentity", value);
        m_defaultPreferredIdentity = value;
        Q_EMIT defaultPreferredIdentityChanged();
    }
}
