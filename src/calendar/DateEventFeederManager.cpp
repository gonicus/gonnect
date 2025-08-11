#include "DateEventFeederManager.h"
#include "ReadOnlyConfdSettings.h"
#include "IDateEventFeederFactory.h"
#include "IDateEventFeeder.h"
#include "NetworkHelper.h"
#include "DateEventManager.h"
#include "SecretPortal.h"
#include "KeychainSettings.h"
#include "ViewHelper.h"

#include <QLoggingCategory>
#include <QPluginLoader>

Q_LOGGING_CATEGORY(lcDateEventFeederManager, "gonnect.app.dateevents.feeder.manager")

DateEventFeederManager::DateEventFeederManager(QObject *parent) : QObject{ parent } { }

void DateEventFeederManager::reload()
{
    DateEventManager::instance().resetDateEvents();
    m_feederConfigIds = m_dateEventFeeders.keys();
    processQueue();
}

void DateEventFeederManager::acquireSecret(const QString &configId,
                                           std::function<void(const QString &)> callback)
{
    auto &secretPortal = SecretPortal::instance();
    if (!secretPortal.isValid()) {
        qCWarning(lcDateEventFeederManager)
                << "Secrets portal is not available - unable to retrieve passwords";
        callback("");
        return;
    }

    if (!secretPortal.isInitialized()) {
        // Retry once secret portal is initialized
        connect(
                &secretPortal, &SecretPortal::initializedChanged, this,
                [this, configId, callback]() { acquireSecret(configId, callback); },
                Qt::ConnectionType::SingleShotConnection);

        return;
    }

    ReadOnlyConfdSettings settings;

    const auto configIdHash = settings.hashForSettingsGroup(configId);
    const auto secretKey = QString("%1_%2").arg(configId, configIdHash);
    const QString secret = KeychainSettings::secret(secretKey);

    if (secret.isEmpty()) {
        auto conn = connect(
                &ViewHelper::instance(), &ViewHelper::passwordResponded, this,
                [secretKey, configId, callback, this](const QString &id, const QString &password) {
                    if (id == configId) {
                        QObject::disconnect(m_viewHelperConnections.value(configId));
                        m_viewHelperConnections.remove(configId);

                        auto &secretPortal = SecretPortal::instance();
                        if (secretPortal.isValid()) {
                            KeychainSettings settings;
                            settings.beginGroup(secretKey);
                            settings.setValue("secret", secretPortal.encrypt(password));
                            settings.endGroup();
                        }

                        callback(password);
                    }
                });

        m_viewHelperConnections.insert(configId, conn);
        settings.beginGroup(configId);
        ViewHelper::instance().requestPassword(configId, settings.value("host", "").toString());
        settings.endGroup();
    } else {
        callback(secretPortal.decrypt(secret));
    }
}

void DateEventFeederManager::initFeederConfigs()
{
    // Event filter options
    QDateTime timeRangeStart = QDateTime::currentDateTime();
    QDateTime timeRangeEnd = timeRangeStart.addDays(3);

    const QObjectList &staticPlugins = QPluginLoader::staticInstances();

    for (QObject *obj : std::as_const(staticPlugins)) {
        if (IDateEventFeederFactory *plugin = qobject_cast<IDateEventFeederFactory *>(obj)) {
            auto const configs = plugin->configurations();
            qCInfo(lcDateEventFeederManager)
                    << "Found" << configs.length() << "active configurations for date event plugin"
                    << plugin->name();

            for (auto &cfg : std::as_const(configs)) {
                m_dateEventFeeders.insert(
                        cfg, plugin->createFeeder(cfg, timeRangeStart, timeRangeEnd, this));
            }
        }
    }
}

void DateEventFeederManager::processQueue()
{
    bool networkAvailable = true;
    auto &networkHelper = NetworkHelper::instance();

    QMutableStringListIterator it(m_feederConfigIds);

    while (it.hasNext()) {
        const auto &configId = it.next();

        if (auto feeder = m_dateEventFeeders.value(configId, nullptr)) {

            QUrl urlToCheck = feeder->networkCheckURL();

            if (!urlToCheck.isEmpty()) {
                if (!networkAvailable) {
                    continue;
                }

                if (!urlToCheck.isValid()) {
                    qCCritical(lcDateEventFeederManager) << "Url is invalid:" << urlToCheck;
                    continue;
                }

                if (!networkHelper.hasConnectivity()) {
                    qCWarning(lcDateEventFeederManager) << "No network connectivity";
                    networkAvailable = false;
                    setupReconnectSignal();
                    continue;
                }

                if (!networkHelper.isReachable(urlToCheck)) {
                    qCWarning(lcDateEventFeederManager)
                            << "Feeder url" << urlToCheck << "is not reachable";
                    setupReconnectSignal();
                    continue;
                }
            }

            feeder->process();
            it.remove();
        }
    }
}

void DateEventFeederManager::setupReconnectSignal()
{
    if (!m_isReconnectSignalSetup) {
        m_isReconnectSignalSetup = true;
        connect(
                &NetworkHelper::instance(), &NetworkHelper::connectivityChanged, this,
                [this]() {
                    m_isReconnectSignalSetup = false;
                    processQueue();
                },
                Qt::ConnectionType::SingleShotConnection);
    }
}
