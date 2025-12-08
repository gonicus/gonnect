#include <QLoggingCategory>

#include "AkonadiEventFeeder.h"
#include "DateEvent.h"
#include "DateEventManager.h"

Q_LOGGING_CATEGORY(lcAkonadiEventFeeder, "gonnect.app.dateevents.feeder.akonadi")

AkonadiEventFeeder::AkonadiEventFeeder(QObject *parent, const QString &source,
                                       const QDateTime &currentTime,
                                       const QDateTime &timeRangeStart,
                                       const QDateTime &timeRangeEnd)
    : QObject(parent),
      m_source(source),
      m_currentTime(currentTime),
      m_timeRangeStart(timeRangeStart),
      m_timeRangeEnd(timeRangeEnd) m_session(new Akonadi::Session("GOnnect::CalendarSession")),
      m_monitor(new Akonadi::Monitor(parent))
{
}

AkonadiEventFeeder::~AkonadiEventFeeder()
{
    delete m_monitor;
    delete m_session;
}

void AkonadiEventFeeder::init()
{
    Akonadi::ItemFetchScope scope;
    scope.fetchFullPayload(true);

    m_monitor->setSession(m_session);
    m_monitor->fetchCollection(true);
    m_monitor->setItemFetchScope(scope);
    m_monitor->setCollectionMonitored(Akonadi::Collection::root());
    m_monitor->setMimeTypeMonitored(KCalendarCore::Event::eventMimeType(), true);

    connect(m_monitor, &Akonadi::Monitor::itemAdded, this, [this](const Akonadi::Item &item) {
        Q_UNUSED(item)

        DateEventManager &manager = DateEventManager::instance();
        manager.removeDateEventsBySource(m_source);

        process();
    });

    connect(m_monitor, &Akonadi::Monitor::itemChanged, this, [this](const Akonadi::Item &item) {
        Q_UNUSED(item)

        DateEventManager &manager = DateEventManager::instance();
        manager.removeDateEventsBySource(m_source);

        process();
    });

    connect(m_monitor, &Akonadi::Monitor::itemRemoved, this, [this](const Akonadi::Item &item) {
        Q_UNUSED(item)

        DateEventManager &manager = DateEventManager::instance();
        manager.removeDateEventsBySource(m_source);

        process();
    });

    process();
}

void AkonadiEventFeeder::process()
{
    m_job = new Akonadi::CollectionFetchJob(Akonadi::Collection::root(),
                                            Akonadi::CollectionFetchJob::Recursive, this);
    m_job->fetchScope().setContentMimeTypes({ KCalendarCore::Event::mimeTypes() });

    connect(m_job, &Akonadi::CollectionFetchJob::finished, this,
            [this](KJob *job) { processCollections(job); });

    m_job->setAutoDelete(true);
    m_job->start();
}

void AkonadiEventFeeder::processCollections(KJob *job)
{
    if (!job || job->error()) {
        qDebug(lcAkonadiEventFeeder) << "Failed to fetch collection tree";
        return;
    }

    Akonadi::CollectionFetchJob *fetchJob = qobject_cast<Akonadi::CollectionFetchJob *>(job);

    const auto &collections = fetchJob->collections();
    for (const auto &collection : collections) {
        if (collection.isValid()) {
            qDebug(lcAkonadiEventFeeder) << collection.name();

            Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(collection);
            job->fetchScope().fetchFullPayload();
            job->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);

            connect(job, &Akonadi::ItemFetchJob::result, this, [this](KJob *job) {
                if (!job || job->error()) {
                    qDebug(lcAkonadiEventFeeder) << "Failed to fetch items";
                    return;
                }

                DateEventManager &manager = DateEventManager::instance();

                const Akonadi::Item::List items =
                        static_cast<Akonadi::ItemFetchJob *>(job)->items();
                for (const auto &item : items) {
                    if (item.hasPayload<KCalendarCore::Event::Ptr>()) {
                        KCalendarCore::Event::Ptr event = item.payload<KCalendarCore::Event::Ptr>();

                        // RRULE
                        bool isRecurrent = event->recurs();
                        KCalendarCore::Recurrence *recurrence = event->recurrence();
                        KCalendarCore::RecurrenceRule *rrule = recurrence->defaultRRule();

                        QString id = event->uid();

                        // RID: The first ever recorded time of a recurrent event instance. We'll
                        // use 'UID-UNIX_TIMESTAMP' as ID.
                        bool isUpdatedRecurrence = false;
                        QDateTime rid = event->recurrenceId().toLocalTime();
                        if (rid.isValid()) {
                            isUpdatedRecurrence = true;
                            id += QString("-%1").arg(rid.toMSecsSinceEpoch());
                        }

                        QDateTime start = event->dtStart().toLocalTime();
                        QDateTime end = event->dtEnd().toLocalTime();

                        // Location
                        QString location = event->location();
                        QString jitsiRoom = manager.getJitsiRoomFromLocation(location);
                        bool isJitsiMeeting = false;
                        bool isOtherLink = false;
                        if (!jitsiRoom.isEmpty()) {
                            location = jitsiRoom;
                            isJitsiMeeting = true;
                        } else if (QUrl(location).isValid()) {
                            isOtherLink = true;
                        }

                        // Status filter
                        bool isCancelled = (event->status()
                                            == KCalendarCore::Incidence::Status::StatusCanceled);

                        // Skip non-recurrent events that are cancelled / outside of our date range
                        // as well as any events without a jitsi meeting as a location
                        if ((start < m_timeRangeStart || start > m_timeRangeEnd
                             || end < m_currentTime || isCancelled)
                            && !isRecurrent && !isUpdatedRecurrence) {
                            continue;
                        }

                        QString summary = event->summary();

                        // Get EXDATE's
                        QList<QDateTime> exdates;
                        for (auto &exdate : recurrence->exDateTimes()) {
                            exdates.append(exdate.toLocalTime());
                        }

                        if (isRecurrent && !isUpdatedRecurrence) {
                            // Recurrent origin event, parsed first
                            qint64 duration = start.secsTo(end);

                            for (auto next = rrule->getNextDate(m_timeRangeStart); next.isValid();
                                 next = rrule->getNextDate(next)) {
                                QDateTime recurStart = next.toLocalTime();
                                QDateTime recurEnd = recurStart.addSecs(duration);
                                if (recurStart > m_timeRangeEnd) {
                                    break;
                                } else if (recurEnd < m_currentTime) {
                                    continue;
                                }

                                if (!exdates.contains(recurStart)
                                    && recurStart >= m_timeRangeStart) {
                                    QString nid = QString("%1-%2").arg(id).arg(
                                            recurStart.toMSecsSinceEpoch());
                                    manager.addDateEvent(new DateEvent(
                                            nid, m_source, recurStart, recurEnd, summary, location,
                                            isJitsiMeeting, isOtherLink));
                                }
                            }
                        } else if (isUpdatedRecurrence) {
                            // Updates of a recurrent event instance
                            if (isCancelled || start < m_timeRangeStart || start > m_timeRangeEnd
                                || end < m_currentTime) {
                                // Updated recurrence doesn't match our criteria anymore
                                manager.removeDateEvent(id);
                            } else if (manager.isAddedDateEvent(id)) {
                                // Exists but modified
                                manager.modifyDateEvent(id, m_source, start, end, summary, location,
                                                        isJitsiMeeting);
                            } else {
                                // Does not exist, e.g. moved from past to future, different day
                                manager.addDateEvent(new DateEvent(id, m_source, start, end,
                                                                   summary, location,
                                                                   isJitsiMeeting, isOtherLink));
                            }
                        } else {
                            // Normal event, no recurrence, or update of a recurrent instance
                            manager.addDateEvent(new DateEvent(id, m_source, start, end, summary,
                                                               location, isJitsiMeeting,
                                                               isOtherLink));
                        }
                    }
                }
            });
        }
    }
}
