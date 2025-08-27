#include <algorithm>

#include "DateEventManager.h"
#include "DateEvent.h"
#include "NotificationManager.h"
#include "ViewHelper.h"
#include "GlobalInfo.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcDateEventManager, "gonnect.app.dateevents.manager")

using namespace std::chrono_literals;

DateEventManager::DateEventManager(QObject *parent) : QObject{ parent }
{
    m_jitsiUrl = QRegularExpression::escape(GlobalInfo::instance().jitsiUrl());
    m_lastCheckedTime = QTime::currentTime();
    m_minuteTimer.setInterval(5s);
    m_minuteTimer.callOnTimeout(this, &DateEventManager::onTimerTimeout);
}

DateEvent *DateEventManager::findDateEventById(const QString &id) const
{
    for (auto dateEvent : std::as_const(m_dateEvents)) {
        if (dateEvent->id() == id) {
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

QString DateEventManager::getJitsiRoomFromLocation(QString location)
{
    if (location.isEmpty()) {
        return "";
    }

    static const QRegularExpression jitsiRoomRegex(QString("%1/(.*)$").arg(m_jitsiUrl),
                                                   QRegularExpression::CaseInsensitiveOption);

    const auto matchResult = jitsiRoomRegex.match(location);
    if (matchResult.hasMatch()) {
        return matchResult.captured(1);
    }

    return "";
}

void DateEventManager::addDateEvent(DateEvent *dateEvent)
{
    if (!dateEvent) {
        qCCritical(lcDateEventManager) << "nullptr received as dateEvent - ignoring";
        return;
    }
    if (isTooOld(*dateEvent)) {
        qCWarning(lcDateEventManager) << "DateEvent is too old and will be ignored";
        return;
    }

    for (const auto &event : std::as_const(m_dateEvents)) {
        if (event && event->id() == dateEvent->id()) {
            qCWarning(lcDateEventManager) << "DateEvent already in list - ignoring";
            return;
        }
    }

    dateEvent->setParent(this);

    qsizetype i = 0;
    for (; i < m_dateEvents.size(); ++i) {
        if (dateEvent->start() < m_dateEvents.at(i)->start()) {
            break;
        }
    }

    m_dateEvents.insert(i, dateEvent);
    emit dateEventAdded(i, dateEvent);

    if (!m_minuteTimer.isActive()) {
        m_minuteTimer.start();
    }
}

void DateEventManager::modifyDateEvent(const QString &id, const QString &source,
                                       const QDateTime &start, const QDateTime &end,
                                       const QString &summary, const QString &roomName,
                                       bool isConfirmed)
{
    QMutableListIterator it(m_dateEvents);
    while (it.hasNext()) {
        const auto event = it.next();
        if (event && event->id() == id) {
            event->setId(id);
            event->setSource(source);
            event->setStart(start);
            event->setEnd(end);
            event->setSummary(summary);
            event->setRoomName(roomName);
            event->setIsConfirmed(isConfirmed);
        }
    }

    // Sort by starting date
    std::sort(m_dateEvents.begin(), m_dateEvents.end(),
              [](const DateEvent *a, const DateEvent *b) { return a->start() < b->start(); });

    emit dateEventModified();
}

void DateEventManager::removeDateEvent(const QString &id)
{
    auto &notMan = NotificationManager::instance();

    qsizetype i = 0;
    QMutableListIterator it(m_dateEvents);
    while (it.hasNext()) {
        i++;
        const auto item = it.next();
        if (item && item->id() == id) {
            QString id = item->id();
            m_alreadyNotifiedDates.remove(id);

            const auto &tag = m_notificationIds[id];
            notMan.remove(tag);
            m_notificationIds.remove(id);

            item->deleteLater();
            it.remove();

            emit dateEventRemoved(i);
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

    qDeleteAll(m_dateEvents);
    m_dateEvents.clear();
    emit dateEventsCleared();
}

void DateEventManager::removeDateEventsBySource(const QString &source)
{
    qsizetype i = 0;
    QMutableListIterator it(m_dateEvents);
    while (it.hasNext()) {
        i++;
        const auto item = it.next();
        if (item && item->source() == source) {
            item->deleteLater();
            it.remove();

            emit dateEventRemoved(i);
        }
    }
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

    for (auto dateEvent : std::as_const(m_dateEvents)) {
        if (dateEvent->roomName() == roomName) {
            foundDateEvent = dateEvent;
            break;
        }
    }

    if (!foundDateEvent) {
        return;
    }

    auto notificationId = m_notificationIds.value(foundDateEvent->id());
    if (notificationId.isEmpty()) {
        return;
    }

    NotificationManager::instance().remove(notificationId);
    m_notificationIds.remove(foundDateEvent->id());
}

void DateEventManager::onTimerTimeout()
{
    auto &notMan = NotificationManager::instance();

    const QTime now = QTime::currentTime();
    if (now.minute() != m_lastCheckedTime.minute()) {
        m_lastCheckedTime = now;

        const QDate today = QDate::currentDate();

        // Checking if new notifications must be created
        for (const auto dateEvent : std::as_const(m_dateEvents)) {
            const auto start = dateEvent->start();
            const auto id = dateEvent->id();
            const auto summary = dateEvent->summary();
            const auto roomName = dateEvent->roomName();

            if (!m_alreadyNotifiedDates.contains(id) && !m_notificationIds.contains(id)
                && start.date() == today && start.time() > now
                && now.secsTo(start.time()) < 2 * 60) {
                auto notification = new Notification(tr("Conference starting soon"), summary,
                                                     Notification::Priority::high, &notMan);

                notification->addButton(tr("Join"), "join-meeting", "", {});
                const auto notificationId = notMan.add(notification);

                connect(notification, &Notification::actionInvoked, this,
                        [this, id, roomName, notificationId](QString action, QVariantList) {
                            if (action == "join-meeting") {
                                NotificationManager::instance().remove(notificationId);
                                m_notificationIds.remove(id);

                                ViewHelper::instance().requestMeeting(roomName);
                            }
                        });

                m_alreadyNotifiedDates.insert(id);
                m_notificationIds.insert(id, notificationId);
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
        auto dateEvent = findDateEventById(it.key());
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
