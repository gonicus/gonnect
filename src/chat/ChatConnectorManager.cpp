#include "ChatConnectorManager.h"
#include "JsChatConnector.h"
#include "JsConnectorConfig.h"
#include "ReadOnlyConfdSettings.h"
#include "Credentials.h"

#include <QRegularExpression>
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

void ChatConnectorManager::saveRecoveryKey(const QString &settingsGroup, const QString &key) const
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

    credentials.set(
            QString("%1/secret").arg(settingsGroup), key, [](bool error, const QString &misc) {
                if (error) {
                    qCCritical(lcChatConnectorManager) << "Error on saving recovery key:" << misc;
                }
            });
}

void ChatConnectorManager::saveAccessToken(const QString &settingsGroup, const QString &token) const
{
    if (!m_isInitialized) {
        qCCritical(lcChatConnectorManager)
                << "Cannot save access token because ChatConnectorManager has not been "
                   "initialized (yet)";
        return;
    }

    auto &credentials = Credentials::instance();
    if (!credentials.isInitialized()) {
        qCCritical(lcChatConnectorManager)
                << "Cannot save access token because Credentials is not initialized (yet)";
        return;
    }

    credentials.set(
            QString("%1/token").arg(settingsGroup), token, [](bool error, const QString &misc) {
                if (error) {
                    qCCritical(lcChatConnectorManager) << "Error on saving access token:" << misc;
                }
            });
}

void ChatConnectorManager::init()
{
    if (m_isInitialized) {
        return;
    }

    m_isInitialized = true;

    ReadOnlyConfdSettings settings;
    static const QRegularExpression groupRegex = QRegularExpression("^jschat[0-9]+$");
    const auto groups = settings.childGroups();

    for (const auto &group : groups) {
        if (groupRegex.match(group).hasMatch()) {
            m_isJsChatAvailable = true;
            settings.beginGroup(group);

            if (!settings.contains("id")) {
                qCCritical(lcChatConnectorManager)
                        << "Chat setting group must have 'id' in group" << group;
                settings.endGroup();
                continue;
            }

            // Retrieve key from secret store
            auto &credentials = Credentials::instance();

            auto config =
                    new JsConnectorConfig{ group,
                                           settings.value("url").toUrl(),
                                           settings.value("id").toString(),
                                           settings.value("deviceId", "GOnnect Client").toString(),
                                           settings.value("displayName", group).toString(),
                                           "",
                                           "" };

            m_waitingCallbackConfigs.insert(group, config);

            if (credentials.isInitialized()) {

                credentials.get(QString("%1/secret").arg(group),
                                [this, group](bool error, const QString &misc) {
                                    if (error) {
                                        qCCritical(lcChatConnectorManager)
                                                << "Error on retrieving recovery key:" << misc;
                                    } else if (!m_waitingCallbackConfigs.contains(group)
                                               || !m_waitingCallbackCount.contains(group)) {
                                        qCCritical(lcChatConnectorManager)
                                                << "Connector listeners are out ouf sync while "
                                                   "retrieving recovery key";
                                    } else {
                                        auto config = m_waitingCallbackConfigs.value(group);
                                        config->recoveryKey = misc;
                                        m_waitingCallbackCount.insert(
                                                group, m_waitingCallbackCount.value(group) - 1);

                                        checkConfigAfterCallback(group);
                                    }
                                });

                credentials.get(QString("%1/token").arg(group),
                                [this, group](bool error, const QString &misc) {
                                    if (error) {
                                        qCCritical(lcChatConnectorManager)
                                                << "Error on retrieving access token:" << misc;
                                    } else if (!m_waitingCallbackConfigs.contains(group)
                                               || !m_waitingCallbackCount.contains(group)) {
                                        qCCritical(lcChatConnectorManager)
                                                << "Connector listeners are out ouf sync while "
                                                   "retrieving access token";
                                    } else {
                                        auto config = m_waitingCallbackConfigs.value(group);
                                        config->accessToken = misc;
                                        m_waitingCallbackCount.insert(
                                                group, m_waitingCallbackCount.value(group) - 1);

                                        checkConfigAfterCallback(group);
                                    }
                                });

                m_waitingCallbackCount.insert(group, 2);
            }

            settings.endGroup();
        }
    }

    std::sort(m_connectors.begin(), m_connectors.end(),
              [](const JsChatConnector *left, const JsChatConnector *right) -> bool {
                  return left->displayName() < right->displayName();
              });

    if (!m_connectors.isEmpty()) {
        Q_EMIT jsChatConnectorsChanged();
    }
}

void ChatConnectorManager::checkConfigAfterCallback(const QString &settingsGroup)
{
    if (!m_waitingCallbackCount.contains(settingsGroup)
        || !m_waitingCallbackConfigs.contains(settingsGroup)) {
        qCCritical(lcChatConnectorManager)
                << "Connector listeners for group" << settingsGroup << "are out ouf sync";
        return;
    }

    if (m_waitingCallbackCount.value(settingsGroup, 1) != 0) {
        // Still waiting for a callback
        return;
    }

    auto config = m_waitingCallbackConfigs.take(settingsGroup);
    m_waitingCallbackCount.remove(settingsGroup);

    m_connectors.append(new JsChatConnector(*config, this));

    delete config;
    config = nullptr;

    std::sort(m_connectors.begin(), m_connectors.end(),
              [](const JsChatConnector *left, const JsChatConnector *right) -> bool {
                  return left->displayName() < right->displayName();
              });

    Q_EMIT jsChatConnectorsChanged();
}
