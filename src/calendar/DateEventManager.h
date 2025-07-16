#pragma once

#include <QObject>
#include <QHash>
#include <QTime>
#include <QTimer>

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

    QString getJitsiRoomFromLocation(QString location);

    /// Add date event; DateEventManager takes ownership of the given object!
    void addDateEvent(DateEvent *dateEvent);

    void modifyDateEvent(const QString &id, const QString &source, const QDateTime &start,
                         const QDateTime &end, const QString &summary, const QString &roomName,
                         bool isConfirmed);
    void removeDateEvent(const QString &id);

    /// Delete all date events
    void resetDateEvents();

    /// Delete events of a specifc source
    void removeDateEventsBySource(const QString &source);

    const QList<DateEvent *> &dateEvents() const { return m_dateEvents; }

    /// Find the DateEvent by the given room name that is currently taking place or nullptr
    DateEvent *currentDateEventByRoomName(const QString &roomName) const;

private slots:
    void onTimerTimeout();

private:
    explicit DateEventManager(QObject *parent = nullptr);

    bool isTooOld(const DateEvent &dateEvent) const;

    QString m_jitsiUrl;
    QList<DateEvent *> m_dateEvents;
    QHash<DateEvent *, QString> m_notificationIds;
    QSet<DateEvent *> m_alreadyNotifiedDates;
    QTimer m_minuteTimer;
    QTime m_lastCheckedTime;
    QDate m_lastCheckedDate;

signals:
    void dateEventAdded(qsizetype index, DateEvent *dateEvent);
    void dateEventModified();
    void dateEventRemoved(qsizetype index);
    void dateEventsCleared();
};
