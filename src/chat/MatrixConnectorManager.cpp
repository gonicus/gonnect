#include "MatrixConnectorManager.h"
#include "KeychainSettings.h"
#include "MatrixConnector.h"
#include "MatrixConnectorConfig.h"
#include "ReadOnlyConfdSettings.h"
#include "SecretPortal.h"

#include <QRegularExpression>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcMatrixConnectorManager, "gonnect.app.chat.matrix.manager")

MatrixConnectorManager::MatrixConnectorManager(QObject *parent) : QObject{ parent }
{
    auto &secretPortal = SecretPortal::instance();
    if (secretPortal.isValid() && !secretPortal.isInitialized()) {
        connect(&secretPortal, &SecretPortal::initializedChanged, this,
                &MatrixConnectorManager::init, Qt::SingleShotConnection);
    } else {
        init();
    }
}

void MatrixConnectorManager::saveRecoveryKey(const QString &settingsGroup, const QString &key) const
{
    if (!m_isInitialized) {
        qCCritical(lcMatrixConnectorManager)
                << "Cannot save recovery key because MatrixConnectorManager has not been "
                   "initialized (yet)";
        return;
    }

    auto &secretPortal = SecretPortal::instance();
    if (!secretPortal.isInitialized()) {
        qCCritical(lcMatrixConnectorManager)
                << "Cannot save recovery key because SecretPortal is not initialized (yet)";
        return;
    }
    if (!secretPortal.isValid()) {
        qCCritical(lcMatrixConnectorManager)
                << "Cannot save recovery key because SecretPortal is not valid";
        return;
    }

    KeychainSettings settings;
    settings.beginGroup(settingsGroup);
    settings.setValue("secret", secretPortal.encrypt(key));
    settings.endGroup();
}

void MatrixConnectorManager::saveAccessToken(const QString &settingsGroup,
                                             const QString &token) const
{
    if (!m_isInitialized) {
        qCCritical(lcMatrixConnectorManager)
                << "Cannot save access token because MatrixConnectorManager has not been "
                   "initialized (yet)";
        return;
    }

    auto &secretPortal = SecretPortal::instance();
    if (!secretPortal.isInitialized()) {
        qCCritical(lcMatrixConnectorManager)
                << "Cannot save access token because SecretPortal is not initialized (yet)";
        return;
    }
    if (!secretPortal.isValid()) {
        qCCritical(lcMatrixConnectorManager)
                << "Cannot save access token because SecretPortal is not valid";
        return;
    }

    KeychainSettings settings;
    settings.beginGroup(settingsGroup);
    settings.setValue("token", secretPortal.encrypt(token));
    settings.endGroup();
}

void MatrixConnectorManager::init()
{
    if (m_isInitialized) {
        return;
    }

    m_isInitialized = true;

    ReadOnlyConfdSettings settings;
    static const QRegularExpression groupRegex = QRegularExpression("^matrix[0-9]+$");
    const auto groups = settings.childGroups();

    for (const auto &group : groups) {
        if (groupRegex.match(group).hasMatch()) {
            m_isMatrixAvailable = true;
            settings.beginGroup(group);

            if (!settings.contains("id")) {
                qCCritical(lcMatrixConnectorManager)
                        << "Matrix setting group must have 'id' in group" << group;
                settings.endGroup();
                continue;
            }

            // Retrieve key from secret store
            auto &secretPortal = SecretPortal::instance();
            QString recoveryKey;
            QString accessToken;

            if (secretPortal.isValid() && secretPortal.isInitialized()) {
                KeychainSettings settings;
                settings.beginGroup(group);
                recoveryKey = settings.value("secret").toString();
                accessToken = settings.value("token").toString();
                settings.endGroup();

                if (!recoveryKey.isEmpty()) {
                    recoveryKey = secretPortal.decrypt(recoveryKey);
                }

                if (!accessToken.isEmpty()) {
                    accessToken = secretPortal.decrypt(accessToken);
                }
            }

            MatrixConnectorConfig config = {
                group,
                settings.value("id").toString(),
                settings.value("deviceId", "GOnnect Client").toString(),
                settings.value("displayName", group).toString(),
                recoveryKey,
                accessToken
            };

            m_connectors.append(new MatrixConnector(config, this));

            settings.endGroup();
        }
    }

    std::sort(m_connectors.begin(), m_connectors.end(),
              [](const MatrixConnector *left, const MatrixConnector *right) -> bool {
                  return left->displayName() < right->displayName();
              });

    if (!m_connectors.isEmpty()) {
        emit matrixConnectorsChanged();
    }
}
