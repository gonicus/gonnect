#pragma once

#include "SecretResponse.h"

#include <QObject>
#include <QTimer>
#include <QTime>
#include <QDateTime>
#include <QHash>

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
    void reloadCalendar();
    void acquireSecret(bool forcePrompt, const QString &configId,
                       std::function<void(SecretResponse response)> callback);

private:
    explicit DateEventFeederManager(QObject *parent = nullptr);

    bool m_isProcessing = false;

    QDateTime m_currentTime;
    QDateTime m_timeRangeStart;
    QDateTime m_timeRangeEnd;
    QDateTime m_nextDayTime;
    qint64 m_nextDayDuration;

    QTimer m_nextDayRefreshTimer;

    void setTimeData();
    void processQueue();
    void requeueConfigId(const QString &configId);
    void setupReconnectSignal();

    QHash<QString, QMetaObject::Connection> m_viewHelperConnections;
    QHash<QString, IDateEventFeeder *> m_dateEventFeeders;
    QStringList m_feederConfigIds;
    bool m_isReconnectSignalSetup = false;
};
