#pragma once

#include <evolution-data-server/libecal/libecal.h>
#include <evolution-data-server/libedata-cal/libedata-cal.h>
#include <glib.h>

#include <QObject>
#include <QDateTime>

#include "IDateEventFeeder.h"

class EDSEventFeeder : public QObject, public IDateEventFeeder
{
    Q_OBJECT

public:
    explicit EDSEventFeeder(QObject *parent = nullptr);
    ~EDSEventFeeder();

    void init(const QString &settingsGroupId, const QString &source,
              const QDateTime &timeRangeStart, const QDateTime &timeRangeEnd);

    virtual void process() override;
    virtual QUrl networkCheckURL() const override { return QUrl(); };

private:
    QDateTime createDateTimeFromTimeType(const ICalTime *datetime);

    void connectEcalClient(ESource *source);

    static void onEcalClientConnected(GObject *source_object, GAsyncResult *result,
                                      gpointer user_data);

    void connectCalendarSignals(ECalClientView *view);

    static void onEventsAdded(ECalClient *client, GSList *components, gpointer user_data);
    static void onEventsModified(ECalClient *client, GSList *components, gpointer user_data);
    static void onEventsRemoved(ECalClient *client, GSList *uids, gpointer user_data);

    void processEventsAdded(ECalClient *client, GSList *components);
    void processEventsModified(ECalClient *client, GSList *components);
    void processEventsRemoved(ECalClient *client, GSList *uids);

    static void onViewCreated(GObject *source_object, GAsyncResult *result, gpointer user_data);

    QString m_source;
    QDateTime m_timeRangeStart;
    QDateTime m_timeRangeEnd;

    ESourceRegistry *m_registry = nullptr;
    GList *m_sources = nullptr;
    gchar *m_searchExpr = nullptr;
    QList<ECalClient *> m_clients;
    QList<ECalClientView *> m_clientViews;
};
