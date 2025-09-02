#pragma once

#include <QObject>
#include <QDateTime>

class JsChatEvent : public QObject
{
    Q_OBJECT

public:
    explicit JsChatEvent(const QString &eventId, const QString &roomId, const QString &senderId,
                         const QDateTime &dateTime, QObject *parent = nullptr);

    QString eventId() const { return m_eventId; }
    QString roomId() const { return m_roomId; }
    QString senderId() const { return m_senderId; }
    QDateTime dateTime() const { return m_dateTime; }

private:
    QString m_eventId;
    QString m_roomId;
    QString m_senderId;
    QDateTime m_dateTime;
};
