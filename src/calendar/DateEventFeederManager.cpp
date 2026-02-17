#include "DateEventFeederManager.h"
#include "ReadOnlyConfdSettings.h"
#include "IDateEventFeederFactory.h"
#include "IDateEventFeeder.h"
#include "NetworkHelper.h"
#include "DateEventManager.h"
#include "Credentials.h"
#include "ViewHelper.h"

#include <QTimer>
#include <QLoggingCategory>
#include <QPluginLoader>

Q_LOGGING_CATEGORY(lcDateEventFeederManager, "gonnect.app.dateevents.feeder.manager")

DateEventFeederManager::DateEventFeederManager(QObject *parent) : QObject{ parent }
{
    setTimeData();

    connect(&m_nextDayRefreshTimer, &QTimer::timeout, this, [this]() {
        setTimeData();
        initFeederConfigs();
        reload();
    });
    m_nextDayRefreshTimer.start();
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

void DateEventFeederManager::reload()
{
    DateEventManager::instance().resetDateEvents();
    m_feederConfigIds = m_dateEventFeeders.keys();
    processQueue();
}

void DateEventFeederManager::acquireSecret(const QString &configId,
                                           std::function<void(const QString &)> callback)
{
    ReadOnlyConfdSettings settings;

    const auto configIdHash = settings.hashForSettingsGroup(configId);
    const auto secretKey = QString("%1_%2").arg(configId, configIdHash);

    Credentials::instance().get(
            secretKey + "/secret",
            [this, configId, secretKey, callback](bool error, const QString &secret) {
                if (error) {
                    qCWarning(lcDateEventFeederManager) << "failed to retrieve secret:" << secret;
                    return;
                }

                if (secret.isEmpty()) {
                    auto &viewHelper = ViewHelper::instance();
                    auto conn = connect(
                            &viewHelper, &ViewHelper::passwordResponded, this,
                            [secretKey, configId, callback, this](const QString &id,
                                                                  const QString &password) {
                                if (id == configId) {
                                    QObject::disconnect(m_viewHelperConnections.value(configId));
                                    m_viewHelperConnections.remove(configId);

                                    Credentials::instance().set(
                                            secretKey + "/secret", password,
                                            [](bool error, const QString &data) {
                                                if (error) {
                                                    qCCritical(lcDateEventFeederManager)
                                                            << "failed to set credentials:" << data;
                                                }
                                            });
                                    callback(password);
                                }
                            });

                    m_viewHelperConnections.insert(configId, conn);

                    ReadOnlyConfdSettings settings;
                    settings.beginGroup(configId);
                    viewHelper.requestPassword(configId, settings.value("host", "").toString());
                    settings.endGroup();
                } else {
                    callback(secret);
                }
            });
}

void DateEventFeederManager::initFeederConfigs()
{
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
                                                               m_timeRangeEnd, this));
            }
        }
    }
}

void DateEventFeederManager::processQueue()
{
    bool networkAvailable = true;
    auto &networkHelper = NetworkHelper::instance();

    if (!m_queueMutex.tryLock()) {
        QTimer::singleShot(100, this, &DateEventFeederManager::processQueue);
        return;
    }

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

            feeder->init();
            it.remove();
        }
    }

    m_queueMutex.unlock();
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
