#pragma once

#include <QObject>
#include <QDateTime>
#include <QTimer>
#include <QHash>
#include <QSet>
#include <QCoroCore>

#include "IDateEventFeeder.h"
#include "DateEventFeederManager.h"
#include "CalDAVEventFeederConfig.h"
#include "WebDavSyncResult.h"

#include <libical/ical.h>
#include <libical/icalerror.h>

class WebDavClient;
struct WebDavItem;

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
    struct CollectionState
    {
        QString syncToken;
        // Normalized resource path -> raw calendar-data
        QHash<QString, QByteArray> calendarData;
    };

    struct PendingCalendar
    {
        QString path;
        QString concreteSource;
    };

    QCoro::Task<> run();

    [[nodiscard]] QCoro::Task<WebDavSyncResult>
    syncCalendarCollection(const QString &collectionPath, int depth);
    [[nodiscard]] QCoro::Task<WebDavSyncResult> syncChildCalendar(const WebDavItem &item,
                                                                  int depth);
    [[nodiscard]] QCoro::Task<WebDavSyncResult> fetchCalendarData(const QString &collectionPath,
                                                                  const QString &requestedPath,
                                                                  const QStringList &hrefs);
    [[nodiscard]] QCoro::Task<WebDavSyncResult> syncCalendarChanges(const QString &collectionPath,
                                                                    const QString &requestedPath,
                                                                    const QString &syncToken);

    QString concreteSourceOf(const QString &collectionPath) const;
    void publishCalendars();
    void processCalendarData(const QString &requestedPath, const QString &concreteSource,
                             uint &successCount, uint &failCount);
    void pruneCalendarData(const QString &requestedPath, const QSet<QString> &existingPaths);
    void pruneVanishedCollections(const QSet<QString> &knownCollectionPaths);

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

    // Calendards currently being loaded but are not published yet
    QList<PendingCalendar> m_pendingCalendars;

    // Normalized collection path -> state (kept for sync-collection)
    QHash<QString, CollectionState> m_collectionStates;

    // Created later - password required first
    std::unique_ptr<WebDavClient> m_webdavClient;
};
