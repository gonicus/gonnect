#include <QLoggingCategory>

#include "AkonadiEventFeeder.h"
#include "DateEvent.h"
#include "DateEventManager.h"

Q_LOGGING_CATEGORY(lcAkonadiEventFeeder, "gonnect.app.dateevents.feeder.akonadi")

AkonadiEventFeeder::AkonadiEventFeeder(QObject *parent, const QString &source,
                                       const QDateTime &timeRangeStart,
                                       const QDateTime &timeRangeEnd)
    : QObject(parent),
      m_source(source),
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

                        QDateTime start = event->dtStart().toLocalTime();

                        QString location = manager.getJitsiRoomFromLocation(event->location());
                        bool isRelevantStatus =
                                (event->status()
                                         == KCalendarCore::Incidence::Status::StatusConfirmed
                                 || event->status()
                                         == KCalendarCore::Incidence::Status::StatusNone);

                        // Skip non-recurrent events that are outside of our date range
                        if (!isRelevantStatus || location.isEmpty()
                            || ((start < m_timeRangeStart || start > m_timeRangeEnd)
                                && !isRecurrent)) {
                            continue;
                        }

                        QDateTime end = event->dtEnd().toLocalTime();

                        QString id = event->uid();
                        QString summary = event->summary();

                        // RID: The first ever recorded time of a recurrent event instance. We'll
                        // use 'UID-UNIX_TIMESTAMP' as ID.
                        bool isUpdatedRecurrence = false;
                        QDateTime rid = event->recurrenceId().toLocalTime();
                        if (rid.isValid()) {
                            isUpdatedRecurrence = true;
                            id += QString("-%1").arg(rid.toMSecsSinceEpoch());
                        }

                        // Get EXDATE's
                        QList<QDateTime> exdates;
                        for (auto &exdate : recurrence->exDateTimes()) {
                            exdates.append(exdate.toLocalTime());
                        }

                        // Recurrent origin event
                        if (isRecurrent && !isUpdatedRecurrence) {
                            qint64 duration = start.secsTo(end);

                            for (auto next = rrule->getNextDate(m_timeRangeStart); next.isValid();
                                 next = rrule->getNextDate(next)) {
                                QDateTime recur = next.toLocalTime();
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
                        } else {
                            // Non-recurrent event or update of a recurrent event instance
                            if (isUpdatedRecurrence && manager.isAddedDateEvent(id)) {
                                manager.modifyDateEvent(id, m_source, start, end, summary, location,
                                                        true);
                            } else {
                                manager.addDateEvent(new DateEvent(id, m_source, start, end,
                                                                   summary, location, true));
                            }
                        }
                    }
                }
            });
        }
    }
}
