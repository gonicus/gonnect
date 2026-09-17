#include "DateEventFeederManager.h"
#include "ReadOnlyConfdSettings.h"
#include "IDateEventFeederFactory.h"
#include "IDateEventFeeder.h"
#include "NetworkHelper.h"
#include "DateEventManager.h"
#include "Credentials.h"
#include "ViewHelper.h"
#include "ErrorBus.h"
#include "SecretResponse.h"

#include <QTimer>
#include <QLoggingCategory>
#include <QPluginLoader>

Q_LOGGING_CATEGORY(lcDateEventFeederManager, "gonnect.app.dateevents.feeder.manager")

using namespace std::chrono_literals;

DateEventFeederManager::DateEventFeederManager(QObject *parent) : QObject{ parent }
{
    setTimeData();

    connect(&m_nextDayRefreshTimer, &QTimer::timeout, this, [this]() {
        setTimeData();
        initFeederConfigs();
        reloadCalendar();
    });
    m_nextDayRefreshTimer.start();

    m_retryTimer.setSingleShot(true);
    m_retryTimer.setInterval(10s);
    m_retryTimer.callOnTimeout(this, [this]() {
        if (m_isReconnectSignalSetup) {
            m_isReconnectSignalSetup = false;
            disconnect(m_connectivityConnection);
            processQueue();
        }
    });
}

void DateEventFeederManager::setTimeData()
{
    // Event filter options
    m_currentTime = QDateTime::currentDateTime();
    m_timeRangeStart = QDateTime(m_currentTime.date(), QTime(0, 0, 0, 0));
    m_timeRangeEnd = m_timeRangeStart.addDays(3);

    m_nextDayTime = m_timeRangeStart.addDays(1);
    m_nextDayDuration = m_currentTime.msecsTo(m_nextDayTime);
    m_nextDayRefreshTimer.setInterval(m_nextDayDuration);
}

void DateEventFeederManager::reloadCalendar()
{
    DateEventManager::instance().resetDateEvents();
    m_feederConfigIds = m_dateEventFeeders.keys();
    processQueue();
}

void DateEventFeederManager::acquireSecret(bool forcePrompt, const QString &configId,
                                           std::function<void(SecretResponse response)> callback)
{
    ReadOnlyConfdSettings settings;

    const auto configIdHash = settings.hashForSettingsGroup(configId);
    const auto secretKey = QString("%1_%2").arg(configId, configIdHash);

    Credentials::instance().get(
            secretKey + "/secret",
            [this, forcePrompt, configId, secretKey,
             callback](QKeychain::Error error, const QString &secret, const QString &) {
                if (error == QKeychain::NoError && !forcePrompt) {
                    callback({ secret });
                } else if (error == QKeychain::Error::EntryNotFound || forcePrompt) {

                    disconnect(m_viewHelperConnections.take(configId));

                    auto &viewHelper = ViewHelper::instance();
                    auto conn =
                            connect(&viewHelper, &ViewHelper::passwordResponded, this,
                                    [secretKey, configId, callback, this](const QString &id,
                                                                          const QString &password) {
                                        if (id == configId) {
                                            disconnect(m_viewHelperConnections.take(configId));

                                            Credentials::instance().set(
                                                    secretKey + "/secret", password,
                                                    [](QKeychain::Error error, const QString &,
                                                       const QString &message) {
                                                        if (error != QKeychain::NoError) {
                                                            ErrorBus::instance().error(
                                                                    tr("Failed to persist calendar "
                                                                       "credentials: %1")
                                                                            .arg(message));
                                                        }
                                                    });
                                            callback({ password });
                                        }
                                    });

                    m_viewHelperConnections.insert(configId, conn);

                    ReadOnlyConfdSettings settings;
                    settings.beginGroup(configId);
                    viewHelper.requestPassword(configId, settings.value("host", "").toString());
                    settings.endGroup();
                } else {
                    callback({ QString(), true });
                }
            });
}

void DateEventFeederManager::initFeederConfigs()
{
    ReadOnlyConfdSettings settings;
    int retryCount = settings.value("generic/feederPluginRetryCount", 5).toInt();
    int retryInterval = settings.value("generic/feederPluginRetryInterval", 10000).toInt();

    const QObjectList &staticPlugins = QPluginLoader::staticInstances();

    for (QObject *obj : std::as_const(staticPlugins)) {
        if (IDateEventFeederFactory *plugin = qobject_cast<IDateEventFeederFactory *>(obj)) {
            auto const configs = plugin->configurations();
            qCInfo(lcDateEventFeederManager)
                    << "Found" << configs.length() << "active configurations for date event plugin"
                    << plugin->name();

            for (auto &cfg : std::as_const(configs)) {
                m_dateEventFeeders.insert(cfg,
                                          plugin->createFeeder(cfg, m_currentTime, m_timeRangeStart,
                                                               m_timeRangeEnd, retryCount,
                                                               retryInterval, this));
            }
        }
    }
}

void DateEventFeederManager::processQueue()
{
    if (m_isProcessing) {
        return;
    }
    m_isProcessing = true;

    bool networkAvailable = true;
    auto &networkHelper = NetworkHelper::instance();

    QMutableStringListIterator it(m_feederConfigIds);
    while (it.hasNext()) {
        const auto &configId = it.next();

        if (auto feeder = m_dateEventFeeders.value(configId, nullptr)) {
            QUrl urlToCheck = feeder->networkCheckURL();

            if (urlToCheck.isEmpty()) {
                feeder->init();
            } else {
                if (!networkAvailable) {
                    continue;
                }

                if (!urlToCheck.isValid()) {
                    qCCritical(lcDateEventFeederManager) << "URL is invalid:" << urlToCheck;

                    continue;
                }

                if (!networkHelper.hasConnectivity()) {
                    qCWarning(lcDateEventFeederManager)
                            << "No connectivity state yet - trying later";

                    networkAvailable = false;
                    setupReconnectSignal();
                    continue;
                }

                networkHelper.isReachable(urlToCheck)
                        .then(this, [this, feeder, configId, urlToCheck](bool isReachable) {
                            if (isReachable) {
                                feeder->init();
                            } else {
                                qCWarning(lcDateEventFeederManager)
                                        << "Feeder URL" << urlToCheck << "is not reachable";

                                requeueConfigId(configId);
                                setupReconnectSignal();
                            }
                        });
            }

            it.remove();
        }
    }

    m_isProcessing = false;
}

void DateEventFeederManager::requeueConfigId(const QString &configId)
{
    if (!m_feederConfigIds.contains(configId)) {
        m_feederConfigIds.append(configId);
    }
}

void DateEventFeederManager::setupReconnectSignal()
{
    if (!m_isReconnectSignalSetup) {
        m_isReconnectSignalSetup = true;

        m_retryTimer.stop();
        m_retryTimer.start();
        disconnect(m_connectivityConnection);

        m_connectivityConnection = connect(
                &NetworkHelper::instance(), &NetworkHelper::connectivityChanged, this,
                [this]() {
                    m_isReconnectSignalSetup = false;
                    processQueue();
                },
                Qt::ConnectionType::SingleShotConnection);
    }
}
