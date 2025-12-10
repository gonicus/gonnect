#pragma once

#include <QObject>
#include <QDateTime>

class DateEvent : public QObject
{
    Q_OBJECT

public:
    explicit DateEvent(const QString &id, const QString &source, const QDateTime &start,
                       const QDateTime &end, const QString &summary, const QString &location,
                       QString &description, QObject *parent = nullptr);

    QString id() const { return m_id; }
    QString source() const { return m_source; }
    QDateTime start() const { return m_start; }
    QDateTime end() const { return m_end; }
    QString summary() const { return m_summary; }
    QString roomName() const { return m_location; }
    QString link() const { return m_link; }
    bool isJitsiMeeting() const { return m_isJitsiMeeting; }
    bool isOtherLink() const { return m_isOtherLink; }

    void setId(const QString &id);
    void setSource(const QString &source);
    void setStart(const QDateTime &start);
    void setEnd(const QDateTime &end);
    void setSummary(const QString &summary);
    void setLocation(const QString &location);
    void setDescription(const QString &description);
    void setIsJitsiMeeting(bool isJitsiMeeting);

    size_t getHash();

private:
    void checkForLinks();

    QString m_id;
    QString m_source;
    QDateTime m_start;
    QDateTime m_end;
    QString m_summary;
    QString m_location;
    QString m_description;
    QString m_link;
    bool m_isJitsiMeeting;
    bool m_isOtherLink;
};

QDebug operator<<(QDebug debug, const DateEvent &dateEvent);
