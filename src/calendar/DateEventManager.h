#pragma once

#include <QObject>
#include <QHash>
#include <QTime>
#include <QTimer>
#include <QMutexLocker>

class DateEvent;

class DateEventManager : public QObject
{
    Q_OBJECT

public:
    static DateEventManager &instance()
    {
        static DateEventManager *_instance = nullptr;
        if (!_instance) {
            _instance = new DateEventManager;
        }
        return *_instance;
    }
    ~DateEventManager();

    QList<QPair<QDateTime, QDateTime>> createDaysFromRange(const QDateTime start,
                                                           const QDateTime end);

    void addDateEvent(const QString &id, const QString &source, const QDateTime &start,
                      const QDateTime &end, const QString &summary, const QString &location,
                      const QString &description);

    void modifyDateEvent(const QString &id, const QString &source, const QDateTime &start,
                         const QDateTime &end, const QString &summary, const QString &location,
                         const QString &description);

    void removeDateEvent(const QString &id, const QDateTime &start, const QDateTime &end);

    /// Delete all date events
    void resetDateEvents();

    /// Delete events of a specifc source
    void removeDateEventsBySource(const QString &source);

    const QList<DateEvent *> &dateEvents() const { return m_dateEvents; }

    bool isAddedDateEvent(const QString &id);

    /// Find the DateEvent by the given room name that is currently taking place or nullptr
    DateEvent *currentDateEventByRoomName(const QString &roomName) const;

    void removeNotificationByRoomName(const QString &roomName);

private Q_SLOTS:
    void onTimerTimeout();

private:
    explicit DateEventManager(QObject *parent = nullptr);

    DateEvent *findDateEventByHash(const size_t &eventHash) const;
    bool isTooOld(const DateEvent &dateEvent) const;
    bool isOver(const DateEvent &dateEvent) const;

    QList<DateEvent *> m_dateEvents;
    QHash<size_t, QString> m_notificationIds;
    QSet<size_t> m_alreadyNotifiedDates;
    QTimer m_minuteTimer;
    QTime m_lastCheckedTime;
    QDate m_lastCheckedDate;

    QMutex m_feederMutex;

Q_SIGNALS:
    void dateEventAdded(qsizetype index, DateEvent *dateEvent);
    void dateEventRemoved(qsizetype index);

    void dateEventsModified();
    void dateEventsCleared();
};
