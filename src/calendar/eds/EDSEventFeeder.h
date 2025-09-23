#pragma once

#include <evolution-data-server/libecal/libecal.h>
#include <evolution-data-server/libedata-cal/libedata-cal.h>
#include <glib.h>

#include <QObject>
#include <QDateTime>
#include <QFuture>
#include <QFutureWatcher>
#include <QPromise>

#include "IDateEventFeeder.h"

class EDSEventFeeder : public QObject, public IDateEventFeeder
{
    Q_OBJECT

public:
    explicit EDSEventFeeder(QObject *parent = nullptr, const QString &source = "",
                            const QDateTime &timeRangeStart = QDateTime(),
                            const QDateTime &timeRangeEnd = QDateTime());
    ~EDSEventFeeder();

    virtual void init() override;
    virtual QUrl networkCheckURL() const override { return QUrl(); };

    void process();

private:
    QDateTime createDateTimeFromTimeType(const ICalTime *datetime);

    static void onEcalClientConnected(GObject *source_object, GAsyncResult *result,
                                      gpointer user_data);

    void connectCalendarSignals(ECalClientView *view);

    static void onEventsAdded(ECalClientView *view, GSList *components, gpointer user_data);
    static void onEventsModified(ECalClientView *view, GSList *components, gpointer user_data);
    static void onEventsRemoved(ECalClientView *view, GSList *uids, gpointer user_data);

    void processEventsAdded(ECalClientView *view);
    void processEventsModified(ECalClientView *view);
    void processEventsRemoved(ECalClientView *view);

    static void onViewCreated(GObject *source_object, GAsyncResult *result, gpointer user_data);

    static void onClientEventsRequested(GObject *source_object, GAsyncResult *result,
                                        gpointer user_data);

    void processEvents(QString clientName, QString clientUid, GSList *components);

    QString m_source;
    QDateTime m_timeRangeStart;
    QDateTime m_timeRangeEnd;

    ESourceRegistry *m_registry = nullptr;
    GList *m_sources = nullptr;
    gchar *m_searchExpr = nullptr;
    QList<ECalClient *> m_clients;
    QList<ECalClientView *> m_clientViews;

    int m_sourceCount = 0;
    std::atomic<int> m_clientCount = 0;
    QPromise<void> *m_sourcePromise = nullptr;
    QFuture<void> m_sourceFuture;
    QFutureWatcher<void> *m_futureWatcher = nullptr;
};
