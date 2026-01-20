#include <algorithm>

#include "DateEventManager.h"
#include "DateEvent.h"
#include "NotificationManager.h"
#include "ViewHelper.h"

#include <QRegularExpression>
#include <QLoggingCategory>
#include <QDesktopServices>

Q_LOGGING_CATEGORY(lcDateEventManager, "gonnect.app.dateevents.manager")

using namespace std::chrono_literals;

DateEventManager::DateEventManager(QObject *parent) : QObject{ parent }
{
    m_lastCheckedTime = QTime::currentTime();
    m_minuteTimer.setInterval(5s);
    m_minuteTimer.callOnTimeout(this, &DateEventManager::onTimerTimeout);
}

QList<QPair<QDateTime, QDateTime>> DateEventManager::createDaysFromRange(const QDateTime start,
                                                                         const QDateTime end)
{
    QList<QPair<QDateTime, QDateTime>> days;

    QDateTime currentStart = start;
    while (currentStart < end) {
        QDateTime endOfDay = currentStart.addDays(1);
        endOfDay.setTime(QTime(0, 0, 0, 0));

        QDateTime currentEnd;
        if (endOfDay < end) {
            currentEnd = endOfDay;
        } else {
            currentEnd = end;
        }

        days.append(qMakePair(currentStart, currentEnd));

        currentStart = endOfDay;
    }

    return days;
}

DateEvent *DateEventManager::findDateEventByHash(const size_t &eventHash) const
{
    for (auto dateEvent : std::as_const(m_dateEvents)) {
        if (dateEvent->getHash() == eventHash) {
            return dateEvent;
        }
    }
    return nullptr;
}

DateEventManager::~DateEventManager()
{
    if (m_minuteTimer.isActive()) {
        m_minuteTimer.stop();
    }

    auto &notMan = NotificationManager::instance();

    for (const auto &tag : std::as_const(m_notificationIds)) {
        notMan.remove(tag);
    }
}

void DateEventManager::addDateEvent(const QString &id, const QString &source,
                                    const QDateTime &start, const QDateTime &end,
                                    const QString &summary, const QString &location,
                                    const QString &description)
{
    auto days = createDaysFromRange(start, end);

    QList<QString> dateEventIds;
    if (days.count() > 1) {
        for (int i = 0; i < days.count(); i++) {
            // Multi-day events will share the same base ID with a counter added to it
            dateEventIds.append(QString("%1-%2").arg(id).arg(i));
        }
    } else {
        dateEventIds.append(id);
    }

    QMutexLocker lock(&m_feederMutex);

    for (int i = 0; i < dateEventIds.count(); i++) {
        const auto eventId = dateEventIds.at(i);
        const auto eventTime = days.at(i);

        auto dateEvent = new DateEvent(eventId, source, eventTime.first, eventTime.second, summary,
                                       location, description);

        if (isTooOld(*dateEvent)) {
            qCWarning(lcDateEventManager) << "DateEvent is too old and will be ignored";
            continue;
        }

        for (const auto &event : std::as_const(m_dateEvents)) {
            if (event && event->id() == dateEvent->id()) {
                qCWarning(lcDateEventManager) << "DateEvent already in list - ignoring";
                break;
            }
        }

        dateEvent->setParent(this);

        qsizetype index = 0;
        for (; index < m_dateEvents.size(); ++index) {
            if (dateEvent->start() < m_dateEvents.at(index)->start()) {
                break;
            }
        }

        m_dateEvents.insert(index, dateEvent);
        Q_EMIT dateEventAdded(index, dateEvent);
    }

    if (!m_minuteTimer.isActive()) {
        m_minuteTimer.start();
    }
}

void DateEventManager::modifyDateEvent(const QString &id, const QString &source,
                                       const QDateTime &start, const QDateTime &end,
                                       const QString &summary, const QString &location,
                                       const QString &description)
{
    auto days = createDaysFromRange(start, end);

    QList<QString> dateEventIds;
    if (days.count() > 1) {
        for (int i = 0; i < days.count(); i++) {
            dateEventIds.append(QString("%1-%2").arg(id).arg(i));
        }
    } else {
        dateEventIds.append(id);
    }

    QMutexLocker lock(&m_feederMutex);

    for (int i = 0; i < dateEventIds.count(); i++) {
        const auto eventId = dateEventIds.at(i);
        const auto eventTime = days.at(i);

        QMutableListIterator it(m_dateEvents);
        while (it.hasNext()) {
            const auto event = it.next();
            if (event && event->id() == eventId) {
                event->setId(eventId);
                event->setSource(source);
                event->setStart(eventTime.first);
                event->setEnd(eventTime.second);
                event->setSummary(summary);
                event->setLocation(location);
                event->setDescription(description);
            }
        }
    }

    // Sort by starting date
    std::sort(m_dateEvents.begin(), m_dateEvents.end(),
              [](const DateEvent *a, const DateEvent *b) { return a->start() < b->start(); });

    Q_EMIT dateEventsModified();
}

void DateEventManager::removeDateEvent(const QString &id, const QDateTime &start,
                                       const QDateTime &end)
{
    auto &notMan = NotificationManager::instance();

    auto days = createDaysFromRange(start, end);

    QList<QString> dateEventIds;
    if (days.count() > 1) {
        for (int i = 0; i < days.count(); i++) {
            dateEventIds.append(QString("%1-%2").arg(id).arg(i));
        }
    } else {
        dateEventIds.append(id);
    }

    QMutexLocker lock(&m_feederMutex);

    for (auto &dateEventId : dateEventIds) {
        qsizetype index = 0;
        QMutableListIterator it(m_dateEvents);
        while (it.hasNext()) {
            index++;
            const auto item = it.next();
            if (item && item->id() == dateEventId) {
                const auto eventHash = item->getHash();
                m_alreadyNotifiedDates.remove(eventHash);

                const auto &tag = m_notificationIds[eventHash];
                notMan.remove(tag);
                m_notificationIds.remove(eventHash);

                item->deleteLater();
                it.remove();

                Q_EMIT dateEventRemoved(index);
            }
        }
    }
}

void DateEventManager::resetDateEvents()
{
    if (m_minuteTimer.isActive()) {
        m_minuteTimer.stop();
    }

    m_alreadyNotifiedDates.clear();

    auto &notMan = NotificationManager::instance();

    for (const auto &tag : std::as_const(m_notificationIds)) {
        notMan.remove(tag);
    }
    m_notificationIds.clear();

    QMutexLocker lock(&m_feederMutex);
    qDeleteAll(m_dateEvents);
    m_dateEvents.clear();
    Q_EMIT dateEventsCleared();
}

void DateEventManager::removeDateEventsBySource(const QString &source)
{
    qsizetype i = 0;
    QMutexLocker lock(&m_feederMutex);
    QMutableListIterator it(m_dateEvents);
    while (it.hasNext()) {
        i++;
        const auto item = it.next();
        if (item && item->source() == source) {
            item->deleteLater();
            it.remove();

            Q_EMIT dateEventRemoved(i);
        }
    }
}

bool DateEventManager::isAddedDateEvent(const QString &id)
{
    // Useful to check if an event instance is a past recurrence,
    // but has been moved into the future
    QMutexLocker lock(&m_feederMutex);
    QMutableListIterator it(m_dateEvents);
    while (it.hasNext()) {
        const auto event = it.next();
        if (event && event->id() == id) {
            return true;
        }
    }

    return false;
}

DateEvent *DateEventManager::currentDateEventByRoomName(const QString &roomName) const
{
    if (roomName.isEmpty()) {
        return nullptr;
    }

    const auto now = QDateTime::currentDateTime();

    for (const auto dateEvent : std::as_const(m_dateEvents)) {
        if (dateEvent->start() < now && now < dateEvent->end()
            && dateEvent->roomName() == roomName) {
            return dateEvent;
        }
    }
    return nullptr;
}

void DateEventManager::removeNotificationByRoomName(const QString &roomName)
{
    DateEvent *foundDateEvent = nullptr;

    QMutexLocker lock(&m_feederMutex);
    for (auto dateEvent : std::as_const(m_dateEvents)) {
        if (dateEvent->roomName() == roomName) {
            foundDateEvent = dateEvent;
            break;
        }
    }

    if (!foundDateEvent) {
        return;
    }

    const auto eventHash = foundDateEvent->getHash();

    // Already joined, do not create a notification in the future
    m_alreadyNotifiedDates.insert(eventHash);

    auto notificationId = m_notificationIds.value(eventHash);
    if (notificationId.isEmpty()) {
        return;
    }

    NotificationManager::instance().remove(notificationId);
    m_notificationIds.remove(eventHash);
}

void DateEventManager::removeNotificationByLink(const QString &link)
{
    DateEvent *foundDateEvent = nullptr;

    QMutexLocker lock(&m_feederMutex);
    for (auto dateEvent : std::as_const(m_dateEvents)) {
        if (dateEvent->link() == link) {
            foundDateEvent = dateEvent;
            break;
        }
    }

    if (!foundDateEvent) {
        return;
    }

    const auto eventHash = foundDateEvent->getHash();

    // Already joined, do not create a notification in the future
    m_alreadyNotifiedDates.insert(eventHash);

    auto notificationId = m_notificationIds.value(eventHash);
    if (notificationId.isEmpty()) {
        return;
    }

    NotificationManager::instance().remove(notificationId);
    m_notificationIds.remove(eventHash);
}

void DateEventManager::onTimerTimeout()
{
    auto &notMan = NotificationManager::instance();
    QMutexLocker lock(&m_feederMutex);

    const QTime now = QTime::currentTime();
    if (now.minute() != m_lastCheckedTime.minute()) {
        m_lastCheckedTime = now;

        const QDate today = QDate::currentDate();

        // Checking if new notifications must be created
        for (const auto dateEvent : std::as_const(m_dateEvents)) {
            const auto eventHash = dateEvent->getHash();
            const auto start = dateEvent->start();
            const auto summary = dateEvent->summary();
            const auto roomName = dateEvent->roomName();
            const auto link = dateEvent->link();
            const auto isJitsiMeeting = dateEvent->isJitsiMeeting();
            const auto isOtherLink = dateEvent->isOtherLink();

            if (!m_alreadyNotifiedDates.contains(eventHash)
                && !m_notificationIds.contains(eventHash) && start.date() == today
                && start.time() > now && now.secsTo(start.time()) < 2 * 60) {
                QString message = isJitsiMeeting ? tr("Conference starting soon")
                                                 : tr("Appointment starting soon");
                auto notification =
                        new Notification(message, summary, Notification::Priority::high, &notMan);

                notification->setIcon(":/icons/gonnect.svg");
                if (isJitsiMeeting) {
                    notification->addButton(tr("Join"), "join-meeting", "", {});
                } else if (isOtherLink) {
                    notification->addButton(tr("Open"), "open-link", "", {});
                }
                const auto notificationId = notMan.add(notification);

                connect(notification, &Notification::actionInvoked, this,
                        [this, eventHash, roomName, link, notificationId](QString action,
                                                                          QVariantList) {
                            if (action == "join-meeting") {
                                NotificationManager::instance().remove(notificationId);
                                m_notificationIds.remove(eventHash);

                                ViewHelper::instance().requestMeeting(roomName);
                            } else if (action == "open-link") {
                                NotificationManager::instance().remove(notificationId);
                                m_notificationIds.remove(eventHash);

                                QDesktopServices::openUrl(link);
                            }
                        });

                m_alreadyNotifiedDates.insert(eventHash);
                m_notificationIds.insert(eventHash, notificationId);
            }
        }
    }

    // Clear DateEvent objects from yesterday and before
    const QDate today = QDate::currentDate();
    if (today != m_lastCheckedDate) {
        m_lastCheckedDate = today;

        QMutableListIterator it(m_dateEvents);
        while (it.hasNext()) {
            const auto item = it.next();
            if (isTooOld(*item)) {
                item->deleteLater();
                it.remove();
            }
        }
    }

    // Clear notifications of events that are over
    QMutableHashIterator it(m_notificationIds);

    while (it.hasNext()) {
        it.next();
        auto dateEvent = findDateEventByHash(it.key());
        if (dateEvent && isOver(*dateEvent)) {
            NotificationManager::instance().remove(it.value());
            it.remove();
        }
    }
}

bool DateEventManager::isTooOld(const DateEvent &dateEvent) const
{
    return dateEvent.start().date() < QDate::currentDate();
}

bool DateEventManager::isOver(const DateEvent &dateEvent) const
{
    return dateEvent.end() < QDateTime::currentDateTime();
}
