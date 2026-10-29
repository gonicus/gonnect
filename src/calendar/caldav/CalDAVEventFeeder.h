#pragma once

#include <QObject>
#include <QDateTime>
#include <QTimer>
#include <QCoroCore>

#include "IDateEventFeeder.h"
#include "DateEventFeederManager.h"
#include "CalDAVEventFeederConfig.h"

#include <libical/ical.h>
#include <libical/icalerror.h>

class WebDavClient;

class CalDAVEventFeeder : public QObject, public IDateEventFeeder
{
    Q_OBJECT

public:
    explicit CalDAVEventFeeder(QObject *parent = nullptr,
                               const CalDAVEventFeederConfig &config = {});

    ~CalDAVEventFeeder();

    void init() override;
    QUrl networkCheckURL() const override;

private:
    QCoro::Task<> run();

    enum class SyncResult { Success, AuthRequired, OtherError };

    QCoro::Task<SyncResult> syncCalendarCollection(const QString &collectionPath, int depth,
                                                   uint &successCount, uint &failCount);

    bool processResponse(const QByteArray &data, const QString &source);
    bool responseDataChanged(const QByteArray &data);
    QDateTime createDateTimeFromTimeType(icaltimetype &datetime);
    void resetCalendar();

    bool m_areWebDavConnectionsInitialized = false;
    bool m_isRunning = false;
    QList<size_t> m_checksums;
    CalDAVEventFeederConfig m_config;
    QTimer m_calendarRefreshTimer;
    QStringList m_concreteSources;

    // Created later - password required first
    std::unique_ptr<WebDavClient> m_webdavClient;
};
