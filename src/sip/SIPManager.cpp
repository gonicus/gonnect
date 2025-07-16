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
#include "AudioManager.h"
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
        epConfig.logConfig.level = m_settings->value("logging/level", 1).toUInt();
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

    m_mediaConfig = new SIPMediaConfig(this);
    m_mediaConfig->applyConfig(epConfig);

    m_uaConfig = new SIPUserAgentConfig(this);
    m_uaConfig->applyConfig(epConfig);

    // Setup log writer
    m_logWriter = new SIPLogWriter();

    pj::LogConfig *log_cfg = &epConfig.logConfig;
    log_cfg->writer = m_logWriter;
    log_cfg->decor = log_cfg->decor
            & ~(::pj_log_decoration::PJ_LOG_HAS_CR | ::pj_log_decoration::PJ_LOG_HAS_NEWLINE
                | ::pj_log_decoration::PJ_LOG_HAS_TIME | ::pj_log_decoration::PJ_LOG_HAS_MICRO_SEC);

    try {
        m_ep.libInit(epConfig);
    } catch (pj::Error &err) {
        qCFatal(lcSIPManager) << "failed to initialize SIP library: " << err.info();
    }

    m_ep.libStart();

    // Load Accounts + Transports
    SIPAccountManager::instance().initialize();
    TogglerManager::instance().initialize();

    // Account initialized - process saved buddy status requests so they get updated
    for (const QString &var : std::as_const(m_buddyStateQueue)) {
        buddyStatus(var);
    }
    m_buddyStateQueue.clear();

    // Create event loop object
    m_ev = new SIPEventLoop(this);

    // Get audio on the road
    AudioManager::instance().initialize();
    VideoManager::instance().updateDevices();

    m_defaultPreferredIdentity = m_settings->value("generic/preferredIdentity", "auto").toString();
    emit defaultPreferredIdentityChanged();
    initializePreferredIdentities();

    if (!isConfigured()) {
        emit notConfigured();
    }
}

bool SIPManager::isConfigured() const
{
    return SIPAccountManager::instance().accounts().length();
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
    identity->setIdentity("+4929329160");
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
            emit defaultPreferredIdentityChanged();
        }
    }

    emit preferredIdentitiesChanged();
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

void SIPManager::shutdown()
{
    qCDebug(lcSIPManager) << "shutting down";

    m_ep.hangupAllCalls();

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
        emit defaultPreferredIdentityChanged();
    }
}
