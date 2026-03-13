#include "EDSEventFeeder.h"
#include "DateEventManager.h"

#include <QMap>
#include <QTimeZone>
#include <QLoggingCategory>
#include <QRegularExpression>

using namespace std::chrono_literals;

Q_LOGGING_CATEGORY(lcEDSEventFeeder, "gonnect.app.dateevents.feeder.eds")

EDSEventFeeder::EDSEventFeeder(QObject *parent, const QString &source, const QDateTime &currentTime,
                               const QDateTime &timeRangeStart, const QDateTime &timeRangeEnd)
    : QObject(parent),
      m_source(source),
      m_currentTime(currentTime),
      m_timeRangeStart(timeRangeStart),
      m_timeRangeEnd(timeRangeEnd)
{
    connect(this, &EDSEventFeeder::feederFailed, this, [this](){
        qCDebug(lcEDSEventFeeder) << "Failed to process EDS sources";

        // Cancel all potentially active EDS async methods
        g_cancellable_cancel(m_cancellable);

        // Disconnect all EDS signal handlers
        disconnectCalendarSignals();

        // Prepare feeder for re-init
        resetFeeder();
        resetContacts();
    });
}

EDSEventFeeder::~EDSEventFeeder()
{
    resetFeeder();
}

void EDSEventFeeder::init()
{
    m_cancellable = g_cancellable_new();

    GError *error = NULL;

    // Create a source registry
    m_registry = e_source_registry_new_sync(m_cancellable, &error);
    if (!m_registry) {
        if (error) {
            qCDebug(lcEDSEventFeeder) << "Can't create registry:" << error->message;
            g_clear_error(&error);
        }
        return;
    }

    // Get the enabled calendar sources of the registry
    m_sources = e_source_registry_list_enabled(m_registry, E_SOURCE_EXTENSION_CALENDAR);
    m_sourceCount = g_list_length(m_sources);
    if (m_sourceCount == 0) {
        qCDebug(lcEDSEventFeeder) << "No sources found in registry";
        return;
    }

    // Calendar event search filter, covers DTSTART component
    // INFO: This is done because the @sexp literally cannot be empty or NULL...
    m_searchExpr = g_strdup("(has-start?)");

    // Clients and signals
    m_sourcePromise = new QPromise<void>();
    m_sourceFuture = m_sourcePromise->future();
    m_futureWatcher = new QFutureWatcher<void>();

    for (GList *iter = m_sources; iter != NULL; iter = g_list_next(iter)) {
        ESource *source = E_SOURCE(iter->data);

        qCDebug(lcEDSEventFeeder) << "Connecting to source" << e_source_get_display_name(source)
                                  << "(" << e_source_get_uid(source) << ")";

        e_cal_client_connect(source, E_CAL_CLIENT_SOURCE_TYPE_EVENTS, -1, m_cancellable,
                             onEcalClientConnected, this);
    }

    m_sourcePromise->start();

    QtFuture::connect(m_futureWatcher, &QFutureWatcher<void>::finished).then([this]() {
        if (m_sourceFuture.isFinished()) {
            process();
        }
    });

    QTimer::singleShot(5s, this, [this]() {
        if (!m_futureWatcher->isFinished()) {
            Q_EMIT feederFailed(); // TODO: Find other points to emit this on failure as well

            m_sourceFuture.cancel();
            m_futureWatcher->cancel();
        }
    });

    m_futureWatcher->setFuture(m_sourceFuture);
}

void EDSEventFeeder::resetFeeder()
{
    g_clear_object(&m_registry);
    if (m_sources) {
        g_list_free_full(m_sources, g_object_unref);
    }
    g_clear_pointer(&m_searchExpr, g_free);
    g_clear_object(&m_cancellable);

    for (auto client : std::as_const(m_clients)) {
        g_clear_object(&client);
    }

    disconnectCalendarSignals();
    for (auto clientView : std::as_const(m_clientViews)) {
        g_clear_object(&clientView);
    }

    if (m_sourcePromise) {
        delete m_sourcePromise;
        m_sourcePromise = nullptr;
    }

    if (m_futureWatcher) {
        m_futureWatcher->deleteLater();
        m_futureWatcher = nullptr;
    }
}

void EDSEventFeeder::resetContacts()
{
    DateEventManager &manager = DateEventManager::instance();

    for (auto client : std::as_const(m_clients)) {
        QString concreteSource = QString("%1-%2").arg(
                m_source, e_source_get_uid(e_client_get_source(E_CLIENT(client))));
        manager.removeDateEventsBySource(concreteSource);
    }
}

void EDSEventFeeder::process()
{
    for (auto client : std::as_const(m_clients)) {
        e_cal_client_get_object_list(client, m_searchExpr, m_cancellable, onClientEventsRequested,
                                     this);
    }
}

QDateTime EDSEventFeeder::createDateTimeFromTimeType(ICalTime *datetime)
{
    if (!datetime) {
        return QDateTime();
    }

    ICalTimezone *zone = i_cal_time_get_timezone(datetime);
    int daylight = i_cal_time_is_daylight(datetime);
    int offset = i_cal_timezone_get_utc_offset(zone, datetime, &daylight);
    QTimeZone convertZone(offset);
    if (zone && convertZone.isValid()) {
        return QDateTime(QDate(i_cal_time_get_year(datetime), i_cal_time_get_month(datetime),
                               i_cal_time_get_day(datetime)),
                         QTime(i_cal_time_get_hour(datetime), i_cal_time_get_minute(datetime),
                               i_cal_time_get_second(datetime)),
                         convertZone)
                .toLocalTime();
    } else {
        return QDateTime(QDate(i_cal_time_get_year(datetime), i_cal_time_get_month(datetime),
                               i_cal_time_get_day(datetime)),
                         QTime(i_cal_time_get_hour(datetime), i_cal_time_get_minute(datetime),
                               i_cal_time_get_second(datetime)));
    }
}

void EDSEventFeeder::onEcalClientConnected(GObject *source_object, GAsyncResult *result,
                                           gpointer user_data)
{
    Q_UNUSED(source_object)

    GError *error = NULL;
    ECalClient *client = NULL;

    EDSEventFeeder *feeder = static_cast<EDSEventFeeder *>(user_data);
    if (feeder) {
        client = E_CAL_CLIENT(e_cal_client_connect_finish(result, &error));
        if (error) {
            qCDebug(lcEDSEventFeeder)
                    << "Can't retrieve finished client connection:" << error->message;
            g_clear_error(&error);
            return;
        }

        e_cal_client_get_view(client, feeder->m_searchExpr, feeder->m_cancellable, onViewCreated,
                              feeder);
    }
}

void EDSEventFeeder::connectViewCompleteSignal(ECalClientView *view)
{
    g_signal_connect(view, "complete", G_CALLBACK(onViewComplete), this);
}

void EDSEventFeeder::onViewComplete(ECalClientView *view, GError *error, gpointer user_data)
{
    /*
        INFO: The "complete" signal is only needed on first startup to wait for the initial
        stream of all objects to the view, we'll disconnect it here.
        Otherwise, future view updates would cause duplicate signal connections.
    */
    guint signalId = g_signal_lookup("complete", G_OBJECT_TYPE(view));
    g_signal_handlers_disconnect_matched(view, G_SIGNAL_MATCH_ID, signalId, 0, NULL, NULL, NULL);

    if (error) {
        qCCritical(lcEDSEventFeeder) << "Failed to wait for view completion, unable to subscribe "
                                        "to live calendar updates:"
                                     << error->message;
        return;
    }

    EDSEventFeeder *feeder = static_cast<EDSEventFeeder *>(user_data);
    if (feeder) {
        feeder->connectCalendarSignals(view);
    }
}

void EDSEventFeeder::connectCalendarSignals(ECalClientView *view)
{
    g_signal_connect(view, "objects-added", G_CALLBACK(onEventsAdded), this);
    g_signal_connect(view, "objects-modified", G_CALLBACK(onEventsModified), this);
    g_signal_connect(view, "objects-removed", G_CALLBACK(onEventsRemoved), this);
}

void EDSEventFeeder::disconnectCalendarSignals()
{
    for (auto view : std::as_const(m_clientViews)) {
        // Match all signals with the same gpointer user_data
        g_signal_handlers_disconnect_matched(view,
                                             G_SIGNAL_MATCH_DATA,
                                             0, 0, NULL, NULL,
                                             this);
    }
}

void EDSEventFeeder::onEventsAdded(ECalClientView *view, GSList *components, gpointer user_data)
{
    // INFO: We want *all* events to account for recursive updates
    Q_UNUSED(components)

    EDSEventFeeder *feeder = static_cast<EDSEventFeeder *>(user_data);
    if (feeder) {
        feeder->processEventsAdded(view);
    }
}

void EDSEventFeeder::onEventsModified(ECalClientView *view, GSList *components, gpointer user_data)
{
    Q_UNUSED(components)

    EDSEventFeeder *feeder = static_cast<EDSEventFeeder *>(user_data);
    if (feeder) {
        feeder->processEventsModified(view);
    }
}

void EDSEventFeeder::onEventsRemoved(ECalClientView *view, GSList *uids, gpointer user_data)
{
    Q_UNUSED(uids)

    EDSEventFeeder *feeder = static_cast<EDSEventFeeder *>(user_data);
    if (feeder) {
        feeder->processEventsRemoved(view);
    }
}

void EDSEventFeeder::processEventsAdded(ECalClientView *view)
{
    DateEventManager &manager = DateEventManager::instance();
    ECalClient *client = e_cal_client_view_ref_client(view);

    if (client) {
        if (!m_clients.contains(client)) {
            g_clear_object(&client);
            return;
        }

        // Use a class-managed reference instead
        const auto idx = m_clients.indexOf(client);
        g_clear_object(&client);

        QString concreteSource = QString("%1-%2").arg(
                m_source, e_source_get_uid(e_client_get_source(E_CLIENT(m_clients.at(idx)))));
        manager.removeDateEventsBySource(concreteSource);

        e_cal_client_get_object_list(m_clients.at(idx), m_searchExpr, m_cancellable,
                                     onClientEventsRequested, this);
    }
}

void EDSEventFeeder::processEventsModified(ECalClientView *view)
{
    DateEventManager &manager = DateEventManager::instance();
    ECalClient *client = e_cal_client_view_ref_client(view);

    if (client) {
        if (!m_clients.contains(client)) {
            g_clear_object(&client);
            return;
        }

        const auto idx = m_clients.indexOf(client);
        g_clear_object(&client);

        QString concreteSource = QString("%1-%2").arg(
                m_source, e_source_get_uid(e_client_get_source(E_CLIENT(m_clients.at(idx)))));
        manager.removeDateEventsBySource(concreteSource);

        e_cal_client_get_object_list(m_clients.at(idx), m_searchExpr, m_cancellable,
                                     onClientEventsRequested, this);
    }
}

void EDSEventFeeder::processEventsRemoved(ECalClientView *view)
{
    DateEventManager &manager = DateEventManager::instance();
    ECalClient *client = e_cal_client_view_ref_client(view);

    if (client) {
        if (!m_clients.contains(client)) {
            g_clear_object(&client);
            return;
        }

        const auto idx = m_clients.indexOf(client);
        g_clear_object(&client);

        QString concreteSource = QString("%1-%2").arg(
                m_source, e_source_get_uid(e_client_get_source(E_CLIENT(m_clients.at(idx)))));
        manager.removeDateEventsBySource(concreteSource);

        e_cal_client_get_object_list(m_clients.at(idx), m_searchExpr, m_cancellable,
                                     onClientEventsRequested, this);
    }
}

void EDSEventFeeder::onViewCreated(GObject *source_object, GAsyncResult *result, gpointer user_data)
{
    GError *error = NULL;
    ECalClient *client = E_CAL_CLIENT(source_object);
    ECalClientView *view = NULL;
    EDSEventFeeder *feeder = static_cast<EDSEventFeeder *>(user_data);

    if (feeder && client) {
        if (!e_cal_client_get_view_finish(client, result, &view, &error)) {
            if (error) {
                qCCritical(lcEDSEventFeeder) << "Can't retrieve finished view:" << error->message;
                g_clear_error(&error);
            }
            return;
        }

        feeder->connectViewCompleteSignal(view);
        e_cal_client_view_start(view, &error);
        if (error) {
            qCCritical(lcEDSEventFeeder) << "Can't start view:" << error->message;
            g_clear_error(&error);
            return;
        }
        feeder->m_clientViews.append(view);

        feeder->m_clients.append(client);
        feeder->m_clientCount++;
        if (feeder->m_clientCount == feeder->m_sourceCount) {
            feeder->m_sourcePromise->finish();
        }
    }
}
void EDSEventFeeder::onClientEventsRequested(GObject *source_object, GAsyncResult *result,
                                             gpointer user_data)
{
    GError *error = NULL;
    GSList *components = NULL;

    EDSEventFeeder *feeder = static_cast<EDSEventFeeder *>(user_data);
    if (feeder) {
        if (!e_cal_client_get_object_list_finish(E_CAL_CLIENT(source_object), result, &components,
                                                 &error)) {
            if (error) {
                qCCritical(lcEDSEventFeeder) << "Can't retrieve events:" << error->message;
                g_clear_error(&error);
            }
            return;
        }
    }

    if (components) {
        feeder->processEvents(
                QString(e_source_get_display_name(e_client_get_source(E_CLIENT(source_object)))),
                QString(e_source_get_uid(e_client_get_source(E_CLIENT(source_object)))),
                components);
    }
}

void EDSEventFeeder::processEvents(QString clientName, QString clientUid, GSList *components)
{
    DateEventManager &manager = DateEventManager::instance();

    QString concreteSource = QString("%1-%2").arg(m_source, clientUid);

    QMap<QString, QList<QDateTime>> exdatesById;

    for (GSList *item = components; item != NULL; item = g_slist_next(item)) {
        ICalComponent *component = I_CAL_COMPONENT(item->data);
        if (component && i_cal_component_isa(component) == I_CAL_VEVENT_COMPONENT) {
            QString id = i_cal_component_get_uid(component);

            ICalTime *dtstart = i_cal_component_get_dtstart(component);
            QDateTime start = createDateTimeFromTimeType(dtstart);

            ICalTime *dtend = i_cal_component_get_dtend(component);
            QDateTime end = createDateTimeFromTimeType(dtend);

            // RRULE
            bool isRecurrent = false;
            ICalProperty *prop =
                    i_cal_component_get_first_property(component, I_CAL_RRULE_PROPERTY);
            ICalRecurrence *rrule = NULL;
            if (prop) {
                isRecurrent = true;
                rrule = i_cal_property_get_rrule(prop);
                g_clear_object(&prop);
            }

            // RID: The first ever recorded time of a recurrent event instance. We'll use
            // 'UID-UNIX_TIMESTAMP' as ID.
            bool isUpdatedRecurrence = false;
            bool isCancelledRecurrence = false;
            ICalTime *rid = i_cal_component_get_recurrenceid(component);
            if (rid && !i_cal_time_is_null_time(rid)) {
                if (exdatesById.value(id).contains(start)) {
                    isCancelledRecurrence = true;
                } else {
                    isUpdatedRecurrence = true;
                    id += QString("-%1").arg(createDateTimeFromTimeType(rid).toMSecsSinceEpoch());
                }
            }

            // Multi-day handling
            bool isMultiDay = start.daysTo(end.addSecs(-1)) > 0 && end > m_currentTime;
            if (isMultiDay && end > m_timeRangeEnd) {
                end = m_timeRangeEnd;
            }

            // Status filter
            ICalPropertyStatus status = i_cal_component_get_status(component);
            bool isCancelled = (status == I_CAL_STATUS_CANCELLED || status == I_CAL_STATUS_FAILED
                                || status == I_CAL_STATUS_DELETED || isCancelledRecurrence);

            // Skip cancelled or non-recurrent events that are outside of our date range
            if (isCancelled
                || (!isRecurrent && !isUpdatedRecurrence
                    && ((start < m_timeRangeStart && !isMultiDay) || start >= m_timeRangeEnd
                        || end < m_currentTime))) {
                continue;
            }

            QString summary = i_cal_component_get_summary(component);
            QString location = i_cal_component_get_location(component);
            QString description = i_cal_component_get_description(component);

            if (isRecurrent) { // Recurrent origin event, parsed first
                // Get EXDATE's
                ICalTime *exdate = NULL;
                QList<QDateTime> exdates;
                for (ICalProperty *prop =
                             i_cal_component_get_first_property(component, I_CAL_EXDATE_PROPERTY);
                     prop != NULL;
                     prop = i_cal_component_get_next_property(component, I_CAL_EXDATE_PROPERTY)) {
                    exdate = i_cal_property_get_exdate(prop);
                    exdates.append(createDateTimeFromTimeType(exdate));
                }
                exdatesById[id] = exdates;

                ICalRecurIterator *recurrenceIter = i_cal_recur_iterator_new(rrule, dtstart);
                if (recurrenceIter) {
                    qint64 duration = start.secsTo(end);

                    for (ICalTime *next = i_cal_recur_iterator_next(recurrenceIter);
                         !i_cal_time_is_null_time(next);
                         next = i_cal_recur_iterator_next(recurrenceIter)) {
                        QDateTime recurStart = createDateTimeFromTimeType(next);
                        QDateTime recurEnd = recurStart.addSecs(duration);
                        if (recurStart >= m_timeRangeEnd) {
                            // Recurrence instances outside of date range
                            break;
                        } else if (recurEnd < m_currentTime) {
                            // Recurrence instance ended earlier today
                            continue;
                        }

                        // Recurrent multi-day handling
                        bool recurMultiDay = recurStart.daysTo(recurEnd.addSecs(-1)) > 0
                                && recurEnd > m_currentTime;
                        if (recurMultiDay && recurEnd > m_timeRangeEnd) {
                            recurEnd = m_timeRangeEnd;
                        }

                        if (!exdates.contains(recurStart)
                            && (recurStart >= m_timeRangeStart || recurMultiDay)) {
                            QString recurId =
                                    QString("%1-%2").arg(id).arg(recurStart.toMSecsSinceEpoch());
                            manager.addDateEvent(recurId, concreteSource, recurStart, recurEnd,
                                                 summary, location, description);
                        }
                    }

                    i_cal_recur_iterator_free(recurrenceIter);
                }
            } else if (isUpdatedRecurrence) { // Updates of a recurrent event instance
                if ((start < m_timeRangeStart && !isMultiDay) || start >= m_timeRangeEnd
                    || end < m_currentTime) {
                    // Updated recurrence doesn't match our criteria anymore
                    manager.removeDateEvent(id, start, end);
                } else if (manager.isAddedDateEvent(id)) {
                    // Exists but modified
                    manager.modifyDateEvent(id, concreteSource, start, end, summary, location,
                                            description);
                } else {
                    // Does not exist, e.g. moved from past to future, different day
                    manager.addDateEvent(id, concreteSource, start, end, summary, location,
                                         description);
                }
            } else { // Normal event, no recurrence, or update of a recurrent instance
                manager.addDateEvent(id, concreteSource, start, end, summary, location,
                                     description);
            }
        }
    }

    g_slist_free_full(components, g_object_unref);
    components = NULL;

    qCInfo(lcEDSEventFeeder) << "Loaded events of source" << clientName << "(" << clientUid << ")";
}
