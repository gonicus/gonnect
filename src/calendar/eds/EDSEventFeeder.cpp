#include "EDSEventFeeder.h"
#include "DateEvent.h"
#include "DateEventManager.h"

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
}

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

    if (m_sourcePromise) {
        delete m_sourcePromise;
        m_sourcePromise = nullptr;
    }

    if (m_futureWatcher) {
        m_futureWatcher->deleteLater();
        m_futureWatcher = nullptr;
    }
}

void EDSEventFeeder::init()
{
    GError *error = nullptr;

    // Create a source registry
    m_registry = e_source_registry_new_sync(nullptr, &error);
    if (!m_registry) {
        if (error) {
            qCDebug(lcEDSEventFeeder) << "Can't create registry:" << error->message;
            g_error_free(error);
            error = nullptr;
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

    for (GList *iter = m_sources; iter != nullptr; iter = g_list_next(iter)) {
        ESource *source = E_SOURCE(iter->data);

        qCDebug(lcEDSEventFeeder) << "Connecting to source" << e_source_get_display_name(source)
                                  << "(" << e_source_get_uid(source) << ")";

        e_cal_client_connect(source, E_CAL_CLIENT_SOURCE_TYPE_EVENTS, -1, nullptr,
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
            qCDebug(lcEDSEventFeeder) << "Failed to process EDS sources";

            m_sourceFuture.cancel();
            m_futureWatcher->cancel();
        }
    });

    m_futureWatcher->setFuture(m_sourceFuture);
}

void EDSEventFeeder::process()
{
    for (auto client : std::as_const(m_clients)) {
        e_cal_client_get_object_list(client, m_searchExpr, nullptr, onClientEventsRequested, this);
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
                    << "Can't retrieve finished client connection:" << error->message;
            g_error_free(error);
            error = nullptr;
            return;
        }

        e_cal_client_get_view(client, feeder->m_searchExpr, nullptr, onViewCreated, feeder);
    }
}

void EDSEventFeeder::connectCalendarSignals(ECalClientView *view)
{
    g_signal_connect(view, "objects-added", G_CALLBACK(onEventsAdded), this);
    g_signal_connect(view, "objects-modified", G_CALLBACK(onEventsModified), this);
    g_signal_connect(view, "objects-removed", G_CALLBACK(onEventsRemoved), this);
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
            g_object_unref(client);
            return;
        }

        // Use a class-managed reference instead
        const auto idx = m_clients.indexOf(client);
        g_object_unref(client);

        QString concreteSource = QString("%1-%2").arg(
                m_source, e_source_get_uid(e_client_get_source(E_CLIENT(m_clients.at(idx)))));
        manager.removeDateEventsBySource(concreteSource);

        e_cal_client_get_object_list(m_clients.at(idx), m_searchExpr, nullptr,
                                     onClientEventsRequested, this);
    }
}

void EDSEventFeeder::processEventsModified(ECalClientView *view)
{
    DateEventManager &manager = DateEventManager::instance();
    ECalClient *client = e_cal_client_view_ref_client(view);

    if (client) {
        if (!m_clients.contains(client)) {
            g_object_unref(client);
            return;
        }

        const auto idx = m_clients.indexOf(client);
        g_object_unref(client);

        QString concreteSource = QString("%1-%2").arg(
                m_source, e_source_get_uid(e_client_get_source(E_CLIENT(m_clients.at(idx)))));
        manager.removeDateEventsBySource(concreteSource);

        e_cal_client_get_object_list(m_clients.at(idx), m_searchExpr, nullptr,
                                     onClientEventsRequested, this);
    }
}

void EDSEventFeeder::processEventsRemoved(ECalClientView *view)
{
    DateEventManager &manager = DateEventManager::instance();
    ECalClient *client = e_cal_client_view_ref_client(view);

    if (client) {
        if (!m_clients.contains(client)) {
            g_object_unref(client);
            return;
        }

        const auto idx = m_clients.indexOf(client);
        g_object_unref(client);

        QString concreteSource = QString("%1-%2").arg(
                m_source, e_source_get_uid(e_client_get_source(E_CLIENT(m_clients.at(idx)))));
        manager.removeDateEventsBySource(concreteSource);

        e_cal_client_get_object_list(m_clients.at(idx), m_searchExpr, nullptr,
                                     onClientEventsRequested, this);
    }
}

void EDSEventFeeder::onViewCreated(GObject *source_object, GAsyncResult *result, gpointer user_data)
{
    GError *error = nullptr;
    ECalClient *client = E_CAL_CLIENT(source_object);
    ECalClientView *view = nullptr;
    EDSEventFeeder *feeder = static_cast<EDSEventFeeder *>(user_data);

    if (feeder && client) {
        if (!e_cal_client_get_view_finish(client, result, &view, &error)) {
            if (error) {
                qCCritical(lcEDSEventFeeder) << "Can't retrieve finished view:" << error->message;
                g_error_free(error);
                error = nullptr;
            }
            return;
        }

        feeder->connectCalendarSignals(view);
        e_cal_client_view_start(view, &error);
        if (error) {
            qCCritical(lcEDSEventFeeder) << "Can't start view:" << error->message;
            g_error_free(error);
            error = nullptr;
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
    GError *error = nullptr;
    GSList *components = nullptr;

    EDSEventFeeder *feeder = static_cast<EDSEventFeeder *>(user_data);
    if (feeder) {
        if (!e_cal_client_get_object_list_finish(E_CAL_CLIENT(source_object), result, &components,
                                                 &error)) {
            if (error) {
                qCCritical(lcEDSEventFeeder) << "Can't retrieve events:" << error->message;
                g_error_free(error);
                error = nullptr;
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

            QString id = i_cal_component_get_uid(component);

            // RID: The first ever recorded time of a recurrent event instance. We'll use
            // 'UID-UNIX_TIMESTAMP' as ID.
            bool isUpdatedRecurrence = false;
            ICalTime *rid = i_cal_component_get_recurrenceid(component);
            if (rid && !i_cal_time_is_null_time(rid)) {
                isUpdatedRecurrence = true;
                id += QString("-%1").arg(createDateTimeFromTimeType(rid).toMSecsSinceEpoch());
            }

            ICalTime *dtstart = i_cal_component_get_dtstart(component);
            QDateTime start = createDateTimeFromTimeType(dtstart);

            ICalTime *dtend = i_cal_component_get_dtend(component);
            QDateTime end = createDateTimeFromTimeType(dtend);

            QString summary = i_cal_component_get_summary(component);
            QString location = i_cal_component_get_location(component);
            QString description = i_cal_component_get_description(component);

            // Status filter
            ICalPropertyStatus status = i_cal_component_get_status(component);
            bool isCancelled = (status == I_CAL_STATUS_CANCELLED || status == I_CAL_STATUS_FAILED
                                || status == I_CAL_STATUS_DELETED || summary.contains("Canceled:"));

            // Skip non-recurrent events that are cancelled / outside of our date range
            // as well as any events without a jitsi meeting as a location
            if ((start < m_timeRangeStart || start > m_timeRangeEnd || end < m_currentTime
                 || isCancelled)
                && !isRecurrent && !isUpdatedRecurrence) {
                continue;
            }

            // Get EXDATE's
            ICalTime *exdate = nullptr;
            QList<QDateTime> exdates;
            for (ICalProperty *prop =
                         i_cal_component_get_first_property(component, I_CAL_EXDATE_PROPERTY);
                 prop != nullptr;
                 prop = i_cal_component_get_next_property(component, I_CAL_EXDATE_PROPERTY)) {
                exdate = i_cal_property_get_exdate(prop);
                exdates.append(createDateTimeFromTimeType(exdate));
            }

            if (isRecurrent && !isUpdatedRecurrence) {
                // Recurrent origin event, parsed first
                ICalRecurIterator *recurrenceIter = i_cal_recur_iterator_new(rrule, dtstart);

                if (recurrenceIter) {
                    qint64 duration = start.secsTo(end);

                    for (ICalTime *next = i_cal_recur_iterator_next(recurrenceIter);
                         !i_cal_time_is_null_time(next);
                         next = i_cal_recur_iterator_next(recurrenceIter)) {
                        QDateTime recurStart = createDateTimeFromTimeType(next);
                        QDateTime recurEnd = recurStart.addSecs(duration);
                        if (recurStart > m_timeRangeEnd) {
                            // Recurrence instances outside of date range
                            break;
                        } else if (recurEnd < m_currentTime) {
                            // Recurrence instance ended earlier today
                            continue;
                        }

                        if (!exdates.contains(recurStart) && !isCancelled
                            && recurStart >= m_timeRangeStart) {
                            QString nid =
                                    QString("%1-%2").arg(id).arg(recurStart.toMSecsSinceEpoch());
                            manager.addDateEvent(new DateEvent(nid, concreteSource, recurStart,
                                                               recurEnd, summary, location,
                                                               description));
                        }
                    }

                    i_cal_recur_iterator_free(recurrenceIter);
                }
            } else if (isUpdatedRecurrence) {
                // Updates of a recurrent event instance
                if (isCancelled || start < m_timeRangeStart || start > m_timeRangeEnd
                    || end < m_currentTime) {
                    // Updated recurrence doesn't match our criteria anymore
                    manager.removeDateEvent(id);
                } else if (manager.isAddedDateEvent(id)) {
                    // Exists but modified
                    manager.modifyDateEvent(id, concreteSource, start, end, summary, location,
                                            description);
                } else {
                    // Does not exist, e.g. moved from past to future, different day
                    manager.addDateEvent(new DateEvent(id, concreteSource, start, end, summary,
                                                       location, description));
                }
            } else {
                // Normal event, no recurrence, or update of a recurrent instance
                manager.addDateEvent(new DateEvent(id, concreteSource, start, end, summary,
                                                   location, description));
            }
        }
    }

    g_slist_free_full(components, g_object_unref);
    components = nullptr;

    qCInfo(lcEDSEventFeeder) << "Loaded events of source" << clientName << "(" << clientUid << ")";
}
