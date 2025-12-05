#pragma once

#include <QObject>
#include <QTimer>
#include <QTime>
#include <QDateTime>
#include <QHash>
#include <QMutex>

class IDateEventFeeder;

class DateEventFeederManager : public QObject
{
    Q_OBJECT

public:
    static DateEventFeederManager &instance()
    {
        static DateEventFeederManager *_instance = nullptr;
        if (!_instance) {
            _instance = new DateEventFeederManager;
        }
        return *_instance;
    }

    void initFeederConfigs();
    void reload();
    void acquireSecret(const QString &configId,
                       std::function<void(const QString &secret)> callback);

private:
    explicit DateEventFeederManager(QObject *parent = nullptr);

    QMutex m_queueMutex;

    QDateTime m_currentTime;
    QDateTime m_timeRangeStart;
    QDateTime m_timeRangeEnd;
    QDateTime m_nextDayTime;
    qint64 m_nextDayDuration;

    QTimer m_nextDayRefreshTimer;

    void setTimeData();
    void processQueue();
    void setupReconnectSignal();

    QHash<QString, QMetaObject::Connection> m_viewHelperConnections;
    QHash<QString, IDateEventFeeder *> m_dateEventFeeders;
    QStringList m_feederConfigIds;
    bool m_isReconnectSignalSetup = false;
};
