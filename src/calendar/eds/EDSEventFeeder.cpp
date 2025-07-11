#include "EDSEventFeeder.h"
#include "DateEvent.h"
#include "DateEventManager.h"

#include <QLoggingCategory>
#include <QRegularExpression>

Q_LOGGING_CATEGORY(lcEDSEventFeeder, "gonnect.app.dateevents.feeder.eds")

EDSEventFeeder::EDSEventFeeder(QObject *parent) : QObject(parent) { }

EDSEventFeeder::~EDSEventFeeder()
{
    if (m_registry) {
        g_object_unref(m_registry);
    }
    if (m_sources) {
        g_list_free_full(m_sources, g_object_unref);
    }
    if (m_searchExpr) {
        g_free(m_searchExpr);
    }

    for (auto client : std::as_const(m_clients)) {
        g_object_unref(client);
        client = nullptr;
    }

    for (auto clientView : std::as_const(m_clientViews)) {
        g_object_unref(clientView);
        clientView = nullptr;
    }
}

void EDSEventFeeder::init(const QString &settingsGroupId, const QString &source,
                          const QDateTime &timeRangeStart, const QDateTime &timeRangeEnd)
{
    Q_UNUSED(settingsGroupId)

    m_source = source;
    m_timeRangeStart = timeRangeStart;
    m_timeRangeEnd = timeRangeEnd;

    GError *error = nullptr;

    // Create a source registry
    m_registry = e_source_registry_new_sync(nullptr, &error);
    if (!m_registry) {
        qCDebug(lcEDSEventFeeder) << "Can't create registry: " << error->message;
        g_error_free(error);
        error = nullptr;
        return;
    }

    // Get the enabled calendar sources of the registry
    m_sources = e_source_registry_list_enabled(m_registry, E_SOURCE_EXTENSION_CALENDAR);
    if (g_list_length(m_sources) == 0) {
        qCDebug(lcEDSEventFeeder) << "No sources found in registry";
        return;
    }

    // Calendar event search filter, covers STATUS component
    // Info: "NOT STARTED" == I_CAL_STATUS_NONE
    m_searchExpr = g_strdup(
            "(or (contains? \"status\" \"CONFIRMED\") (contains? \"status\" \"NOT STARTED\"))");

    // Clients and signals
    for (GList *iter = m_sources; iter != nullptr; iter = g_list_next(iter)) {
        ESource *source = E_SOURCE(iter->data);

        const gchar *id = e_source_get_uid(source);
        const gchar *dn = e_source_get_display_name(source);

        qCDebug(lcEDSEventFeeder) << "Connecting to '" << id << "' (" << dn << ")";

        connectEcalClient(source);
    }
}

void EDSEventFeeder::process()
{
    GError *error = nullptr;
    GSList *components = nullptr;

    DateEventManager &manager = DateEventManager::instance();

    for (auto client : std::as_const(m_clients)) {
        const gchar *id = e_source_get_uid(e_client_get_source(E_CLIENT(client)));
        const gchar *dn = e_source_get_display_name(e_client_get_source(E_CLIENT(client)));

        if (!e_cal_client_get_object_list_sync(client, m_searchExpr, &components, nullptr,
                                               &error)) {
            qCDebug(lcEDSEventFeeder)
                    << "Can't get events of '" << id << "' (" << dn << "), skipping...";
            g_error_free(error);
            error = nullptr;
        } else {
            for (GSList *item = components; item != nullptr; item = g_slist_next(item)) {
                ICalComponent *component = I_CAL_COMPONENT(item->data);
                if (component && i_cal_component_isa(component) == I_CAL_VEVENT_COMPONENT) {
                    // RRULE
                    bool isRecurrent = false;
                    ICalProperty *prop =
                            i_cal_component_get_first_property(component, I_CAL_RRULE_PROPERTY);
                    ICalRecurrence *rrule = nullptr;
                    if (prop) {
                        isRecurrent = true;
                        rrule = i_cal_property_get_rrule(prop);
                        g_clear_object(&prop);
                    }

                    ICalTime *dtstart = i_cal_component_get_dtstart(component);
                    QDateTime start = createDateTimeFromTimeType(dtstart);

                    QString location = manager.getJitsiRoomFromLocation(
                            i_cal_component_get_location(component));

                    // Skip non-recurrent events that are outside of our date range
                    if (location.isEmpty()
                        || ((start < m_timeRangeStart || start > m_timeRangeEnd) && !isRecurrent)) {
                        continue;
                    }

                    ICalTime *dtend = i_cal_component_get_dtend(component);
                    QDateTime end = createDateTimeFromTimeType(dtend);

                    QString id = i_cal_component_get_uid(component);
                    QString summary = i_cal_component_get_summary(component);

                    // RID: The first ever recorded time of a recurrent event instance. We'll use
                    // 'UID-UNIX_TIMESTAMP' as ID.
                    bool isUpdatedRecurrence = false;
                    ICalTime *rid = i_cal_component_get_recurrenceid(component);
                    if (rid && !i_cal_time_is_null_time(rid)) {
                        isUpdatedRecurrence = true;
                        id += QString("-%1").arg(
                                createDateTimeFromTimeType(rid).toMSecsSinceEpoch());
                    }

                    // Get EXDATE's
                    ICalTime *exdate = nullptr;
                    QList<QDateTime> exdates;
                    for (ICalProperty *prop = i_cal_component_get_first_property(
                                 component, I_CAL_EXDATE_PROPERTY);
                         prop != nullptr; prop = i_cal_component_get_next_property(
                                                  component, I_CAL_EXDATE_PROPERTY)) {
                        exdate = i_cal_property_get_exdate(prop);
                        exdates.append(createDateTimeFromTimeType(exdate));
                    }

                    // Recurrent origin event
                    if (isRecurrent && !isUpdatedRecurrence) {
                        ICalRecurIterator *recurrenceIter =
                                i_cal_recur_iterator_new(rrule, dtstart);

                        if (recurrenceIter) {
                            qint64 duration = start.secsTo(end);

                            for (ICalTime *next = i_cal_recur_iterator_next(recurrenceIter);
                                 !i_cal_time_is_null_time(next);
                                 next = i_cal_recur_iterator_next(recurrenceIter)) {
                                QDateTime recur = createDateTimeFromTimeType(next);
                                if (recur > m_timeRangeEnd) {
                                    break;
                                }

                                if (!exdates.contains(recur) && recur >= m_timeRangeStart) {
                                    QString nid =
                                            QString("%1-%2").arg(id).arg(recur.toMSecsSinceEpoch());
                                    manager.addDateEvent(new DateEvent(nid, m_source, recur,
                                                                       recur.addMSecs(duration),
                                                                       summary, location, true));
                                }
                            }

                            i_cal_recur_iterator_free(recurrenceIter);
                        }
                    } else {
                        // Non-recurrent event or update of a recurrent event instance
                        if (isUpdatedRecurrence) {
                            manager.modifyDateEvent(id, m_source, start, end, summary, location,
                                                    true);
                        } else {
                            manager.addDateEvent(new DateEvent(id, m_source, start, end, summary,
                                                               location, true));
                        }
                    }
                }
            }

            g_slist_free_full(components, g_object_unref);
            components = nullptr;
        }
    }
}

QDateTime EDSEventFeeder::createDateTimeFromTimeType(const ICalTime *datetime)
{
    if (!datetime) {
        return QDateTime();
    }

    QString zone = i_cal_time_get_tzid(datetime);
    if (zone == "UTC") {
        return QDateTime::fromString(QString("%1-%2-%3T%4:%5:%6Z")
                                             .arg(i_cal_time_get_year(datetime), 4, 10, '0')
                                             .arg(i_cal_time_get_month(datetime), 2, 10, '0')
                                             .arg(i_cal_time_get_day(datetime), 2, 10, '0')
                                             .arg(i_cal_time_get_hour(datetime), 2, 10, '0')
                                             .arg(i_cal_time_get_minute(datetime), 2, 10, '0')
                                             .arg(i_cal_time_get_second(datetime), 2, 10, '0'),
                                     Qt::ISODate)
                .toLocalTime();
    } else {
        return QDateTime(QDate(i_cal_time_get_year(datetime), i_cal_time_get_month(datetime),
                               i_cal_time_get_day(datetime)),
                         QTime(i_cal_time_get_hour(datetime), i_cal_time_get_minute(datetime),
                               i_cal_time_get_second(datetime)));
    }
}

void EDSEventFeeder::connectEcalClient(ESource *source)
{
    e_cal_client_connect(source, E_CAL_CLIENT_SOURCE_TYPE_EVENTS, -1, nullptr,
                         onEcalClientConnected, this);
}

void EDSEventFeeder::onEcalClientConnected(GObject *source_object, GAsyncResult *result,
                                           gpointer user_data)
{
    Q_UNUSED(source_object)

    GError *error = nullptr;
    ECalClient *client = nullptr;

    EDSEventFeeder *feeder = static_cast<EDSEventFeeder *>(user_data);
    if (feeder) {
        client = E_CAL_CLIENT(e_cal_client_connect_finish(result, &error));
        if (error) {
            qCDebug(lcEDSEventFeeder)
                    << "Can't retrieve finished client connection: " << error->message;
            g_error_free(error);
            error = nullptr;
        }

        e_cal_client_get_view(client, feeder->m_searchExpr, nullptr, onViewCreated, feeder);
        feeder->m_clients.append(client);
    }
}

void EDSEventFeeder::connectCalendarSignals(ECalClientView *view)
{
    g_signal_connect(view, "objects-added", G_CALLBACK(onEventsAdded), this);
    g_signal_connect(view, "objects-modified", G_CALLBACK(onEventsModified), this);
    g_signal_connect(view, "objects-removed", G_CALLBACK(onEventsRemoved), this);
}

void EDSEventFeeder::onEventsAdded(ECalClient *client, GSList *components, gpointer user_data)
{
    EDSEventFeeder *feeder = static_cast<EDSEventFeeder *>(user_data);
    if (feeder) {
        feeder->processEventsAdded(client, components);
    }
}

void EDSEventFeeder::onEventsModified(ECalClient *client, GSList *components, gpointer user_data)
{
    EDSEventFeeder *feeder = static_cast<EDSEventFeeder *>(user_data);
    if (feeder) {
        feeder->processEventsModified(client, components);
    }
}

void EDSEventFeeder::onEventsRemoved(ECalClient *client, GSList *uids, gpointer user_data)
{
    EDSEventFeeder *feeder = static_cast<EDSEventFeeder *>(user_data);
    if (feeder) {
        feeder->processEventsRemoved(client, uids);
    }
}

void EDSEventFeeder::processEventsAdded(ECalClient *client, GSList *components)
{
    Q_UNUSED(client)
    Q_UNUSED(components)

    DateEventManager &manager = DateEventManager::instance();
    manager.removeDateEventsBySource(m_source);

    process();
}

void EDSEventFeeder::processEventsModified(ECalClient *client, GSList *components)
{
    Q_UNUSED(client)
    Q_UNUSED(components)

    DateEventManager &manager = DateEventManager::instance();
    manager.removeDateEventsBySource(m_source);

    process();
}

void EDSEventFeeder::processEventsRemoved(ECalClient *client, GSList *uids)
{
    Q_UNUSED(client)
    Q_UNUSED(uids)

    DateEventManager &manager = DateEventManager::instance();
    manager.removeDateEventsBySource(m_source);

    process();
}

void EDSEventFeeder::onViewCreated(GObject *source_object, GAsyncResult *result, gpointer user_data)
{
    GError *error = nullptr;
    ECalClientView *view = nullptr;

    EDSEventFeeder *feeder = static_cast<EDSEventFeeder *>(user_data);
    if (feeder) {
        if (!e_cal_client_get_view_finish(E_CAL_CLIENT(source_object), result, &view, &error)) {
            qCDebug(lcEDSEventFeeder) << "Can't retrieve finished view: " << error->message;
            g_error_free(error);
            error = nullptr;
            return;
        }

        feeder->m_clientViews.append(view);
        feeder->connectCalendarSignals(view);
        e_cal_client_view_start(view, &error);
        if (error) {
            qCDebug(lcEDSEventFeeder) << "Can't start view: " << error->message;
            g_error_free(error);
            error = nullptr;
        }
    }
}
