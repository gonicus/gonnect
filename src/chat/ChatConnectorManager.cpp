#include "ChatConnectorManager.h"
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
}
