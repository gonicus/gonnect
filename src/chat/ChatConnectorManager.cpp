#include "ChatConnectorManager.h"
#include "ChatMessageSearchProvider.h"
#include "Credentials.h"
#include "IChatProvider.h"
#include "IpcDispatcher.h"
#include "ReadOnlyConfdSettings.h"
#include "ViewHelper.h"
#include "SecretGenerator.h"
#include "ErrorBus.h"

#include <QRegularExpression>
#include <QCryptographicHash>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcChatConnectorManager, "gonnect.app.chat.connectorManager")

ChatConnectorManager::ChatConnectorManager(QObject *parent) : QObject{ parent }
{
    auto &credentials = Credentials::instance();

    if (!credentials.isInitialized()) {
        connect(&credentials, &Credentials::initializedChanged, this, &ChatConnectorManager::init,
                Qt::SingleShotConnection);
    } else {
        init();
    }
}

void ChatConnectorManager::saveSecret(const QString &settingsGroup, const QString &secret) const
{
    if (!m_isInitialized) {
        qCCritical(lcChatConnectorManager)
                << "Cannot save recovery key because ChatConnectorManager has not been "
                   "initialized (yet)";
        return;
    }

    auto &credentials = Credentials::instance();
    if (!credentials.isInitialized()) {
        qCCritical(lcChatConnectorManager)
                << "Cannot save recovery key because Credentials is not initialized (yet)";
        return;
    }

    credentials.set(QString("%1/secret").arg(settingsGroup), secret,
                    [](QKeychain::Error error, const QString &, const QString &message) {
                        if (error != QKeychain::NoError) {
                            ErrorBus::instance().error(
                                    tr("Failed to persist chat recovery code: %1").arg(message));
                        }
                    });
}

QString ChatConnectorManager::hashForSettingsGroup(const QString &group)
{
    ReadOnlyConfdSettings settings;
    settings.beginGroup(group);

    auto childKeys = settings.childKeys();

    if (!childKeys.contains("backendUrl") || !childKeys.contains("loginFlow")
        || !childKeys.contains("userId")) {
        qCCritical(lcChatConnectorManager) << "Chat config group" << group
                                           << "must have keys backendUrl, loginFlow and userId";
        return "";
    }

    const QString str = QString("backendUrl=%1;userId=%2;loginFlow=%3")
                                .arg(settings.value("backendUrl").toString(),
                                     settings.value("userId").toString(),
                                     settings.value("loginFlow").toString());

    return QCryptographicHash::hash(str.toUtf8(), QCryptographicHash::Md5).toHex();
}

ChatConnectorManager::~ChatConnectorManager()
{
    qDeleteAll(m_groupStates);
}

void ChatConnectorManager::init()
{
    if (m_isInitialized) {
        return;
    }

    // INFO: We're hooking up the message search provider early to get notified
    // about all provider/room changes
    // Warn: No ref tracking atm, thus no destruction...
    (void)ChatMessageSearchProvider::instance();

    static const QRegularExpression groupRegex = QRegularExpression("^chat[0-9]+$");
    ReadOnlyConfdSettings settings;
    const auto groups = settings.childGroups();

    for (const auto &group : groups) {
        if (groupRegex.match(group).hasMatch()) {
            processSettingGroup(group);
        }
    }

    m_isInitialized = true;
}

void ChatConnectorManager::processSettingGroup(const QString &group)
{
    auto groupState = m_groupStates.value(group, nullptr);
    if (!groupState) {
        groupState = new SettingGroupStates;
        groupState->config = new IpcConfig;

        ReadOnlyConfdSettings settings;
        settings.beginGroup(group);

        groupState->config->configHash = hashForSettingsGroup(group);
        groupState->config->displayName = settings.value("displayName", "GOnnect").toString();
        groupState->config->userId = settings.value("userId").toString();
        groupState->config->backendUrl = settings.value("backendUrl").toUrl();
        groupState->config->identityProviderId = settings.value("identityProviderId").toString();
        groupState->config->idConvRegexpString = settings.value("idConvRegexpString").toString();
        groupState->config->idConvReplacementString =
                settings.value("idConvReplacementString").toString();

        const auto flowStr = settings.value("loginFlow").toString();
        if (flowStr == "Credentials") {
            groupState->config->loginFlow = IpcConfig::LoginFlow::Credentials;
        } else if (flowStr == "SSO") {
            groupState->config->loginFlow = IpcConfig::LoginFlow::SSO;
        }

        if (!IpcDispatcher::checkConfig(*groupState->config)) {
            qCCritical(lcChatConnectorManager)
                    << "Errors in config for" << group << "found - aborting";
            delete groupState;
            return;
        }

        m_groupStates.insert(group, groupState);

        // Login secret
        groupState->isLoginSecretRequired = IpcDispatcher::requiresSecret(*groupState->config);

        if (groupState->isLoginSecretRequired) {
            if (!groupState->isLoginSecretInitialized
                && !groupState->isWaitingForLoginSecretResponse) {
                groupState->isWaitingForLoginSecretResponse = true;

                auto &credentials = Credentials::instance();
                Q_ASSERT(credentials.isInitialized());

                credentials.get(
                        QString("%1/secret").arg(group),
                        [this, group](QKeychain::Error error, const QString &misc,
                                      const QString &message) {
                            if (error) {
                                qCCritical(lcChatConnectorManager)
                                        << QString("Error on retrieving secret for %1: %2")
                                                   .arg(group, message);
                                ErrorBus::instance().error(tr("Failed to receive secret for %1: %2")
                                                                   .arg(group, message));
                            } else if (auto state = m_groupStates.value(group, nullptr)) {
                                if (!misc.isEmpty()) {
                                    state->config->secret = misc;
                                    state->isWaitingForLoginSecretResponse = false;
                                    state->isLoginSecretInitialized = true;
                                    processSettingGroup(group);
                                } else {
                                    // No secret in credentials service -> ask user for it
                                    auto &viewHelper = ViewHelper::instance();

                                    QMetaObject::Connection conn;
                                    conn = connect(
                                            &viewHelper, &ViewHelper::passwordResponded, this,
                                            [this, conn, group](const QString id,
                                                                const QString password) {
                                                if (id == group) {
                                                    disconnect(conn);

                                                    if (auto state = m_groupStates.value(group,
                                                                                         nullptr)) {
                                                        state->config->secret = password;
                                                        state->isLoginSecretInitialized = true;
                                                        state->isWaitingForLoginSecretResponse =
                                                                false;
                                                        processSettingGroup(group);
                                                    }
                                                }
                                            });

                                    viewHelper.requestPassword(
                                            group,
                                            state->config->backendUrl.isEmpty()
                                                    ? group
                                                    : state->config->backendUrl.toString());
                                }
                            }
                        });
            }
        }

        // Encryption secret
        if (!groupState->isEncryptionSecretInitialized
            && !groupState->isWaitingForEncryptionSecretResponse) {
            groupState->isWaitingForEncryptionSecretResponse = true;

            auto &credentials = Credentials::instance();
            Q_ASSERT(credentials.isInitialized());
            credentials.get(
                    QString("%1/encryptionSecret").arg(group),
                    [this, group](QKeychain::Error error, const QString &secret,
                                  const QString &message) {
                        if (error) {
                            qCCritical(lcChatConnectorManager)
                                    << QString("Error on retrieving secret for %1: %2")
                                               .arg(group, secret);
                            ErrorBus::instance().error(
                                    tr("Failed to receive secret for %1: %2").arg(group, message));
                        } else if (auto state = m_groupStates.value(group, nullptr)) {
                            if (!secret.isEmpty()) {
                                state->config->encryptionSecret = secret;
                                state->isEncryptionSecretInitialized = true;
                                state->isWaitingForEncryptionSecretResponse = false;
                                processSettingGroup(group);
                            } else {
                                const auto secret = SecretGenerator::generateSecret(32);
                                state->config->encryptionSecret = secret;

                                Credentials::instance().set(
                                        QString("%1/encryptionSecret").arg(group), secret,
                                        [this, group](QKeychain::Error error, const QString &,
                                                      const QString &message) {
                                            if (error) {
                                                qCCritical(lcChatConnectorManager)
                                                        << "Error while saving encryptionSecret:"
                                                        << message;
                                                ErrorBus::instance().error(
                                                        tr("Failed to save secret for %1: %2")
                                                                .arg(group, message));
                                            } else if (auto state = m_groupStates.value(group,
                                                                                        nullptr)) {
                                                state->isEncryptionSecretInitialized = true;
                                                state->isWaitingForEncryptionSecretResponse = false;
                                                processSettingGroup(group);
                                            }
                                        });
                            }
                        }
                    });
        }

        // Persistent storage secret
        if (!groupState->isPersistentSecretInitialized
            && !groupState->isWaitingForPersistentSecretResponse) {
            groupState->isWaitingForPersistentSecretResponse = true;

            auto &credentials = Credentials::instance();
            Q_ASSERT(credentials.isInitialized());
            credentials.get(
                    QString("%1/persistentStorageSecret").arg(group),
                    [this, group](QKeychain::Error error, const QString &secret,
                                  const QString &message) {
                        if (error) {
                            qCCritical(lcChatConnectorManager)
                                    << QString("Error on retrieving secret for %1: %2")
                                               .arg(group, message);
                            ErrorBus::instance().error(
                                    tr("Failed to receive secret for %1: %2").arg(group, message));
                        } else if (auto state = m_groupStates.value(group, nullptr)) {
                            if (!secret.isEmpty()) {
                                state->config->persistentStorageSecret = secret;
                                state->isPersistentSecretInitialized = true;
                                state->isWaitingForPersistentSecretResponse = false;
                                processSettingGroup(group);

                            } else {
                                const auto secret = SecretGenerator::generateSecret(32);
                                state->config->persistentStorageSecret = secret;

                                Credentials::instance().set(
                                        QString("%1/persistentStorageSecret").arg(group), secret,
                                        [this, group](QKeychain::Error error, const QString &,
                                                      const QString &message) {
                                            if (error) {
                                                qCCritical(lcChatConnectorManager)
                                                        << "Error while saving "
                                                           "persistentStorageSecret:"
                                                        << message;
                                                ErrorBus::instance().error(
                                                        tr("Failed to store persistent storage "
                                                           "secret for %1: %2")
                                                                .arg(group, message));
                                            } else if (auto state = m_groupStates.value(group,
                                                                                        nullptr)) {
                                                state->isPersistentSecretInitialized = true;
                                                state->isWaitingForPersistentSecretResponse = false;
                                                processSettingGroup(group);
                                            }
                                        });
                            }
                        }
                    });
        }
    }

    // Check if everything is initalized and create provider object
    if (groupState->isEncryptionSecretInitialized && groupState->isPersistentSecretInitialized
        && (!groupState->isLoginSecretRequired || groupState->isLoginSecretInitialized)) {
        IChatProvider *provider = new IpcDispatcher(group, *groupState->config, this);
        m_chatProviders.append(provider);
        m_groupStates.remove(group);
        delete groupState;

        connect(provider, &IChatProvider::unreadNotificationsCountChanged, this,
                &ChatConnectorManager::updateUnreadNotificationsCount);

        Q_EMIT isChatAvailableChanged();
        Q_EMIT chatConnectorsChanged();

        updateUnreadNotificationsCount();
    }
}

void ChatConnectorManager::updateUnreadNotificationsCount()
{
    qsizetype count = 0;

    for (const IChatProvider *provider : std::as_const(m_chatProviders)) {
        count += provider->unreadNotificationsCount();
    }

    if (m_unreadNotificationsCount != count) {
        m_unreadNotificationsCount = count;
        Q_EMIT unreadNotificationsCountChanged();
    }
}

ChatConnectorManager::SettingGroupStates::~SettingGroupStates()
{
    if (config) {
        delete config;
        config = nullptr;
    }
}
