#include "DateEvent.h"

#include <QHash>

DateEvent::DateEvent(const QString &id, const QString &source, const QDateTime &start,
                     const QDateTime &end, const QString &summary, const QString &roomName,
                     QObject *parent)
    : QObject{ parent },
      m_id{ id },
      m_source{ source },
      m_start{ start },
      m_end{ end },
      m_summary{ summary },
      m_roomName{ roomName }
{
}

void DateEvent::setId(const QString &id)
{
    m_id = id;
}

void DateEvent::setSource(const QString &source)
{
    m_source = source;
}

void DateEvent::setStart(const QDateTime &start)
{
    m_start = start;
}

void DateEvent::setEnd(const QDateTime &end)
{
    m_end = end;
}

void DateEvent::setSummary(const QString &summary)
{
    m_summary = summary;
}

void DateEvent::setRoomName(const QString &roomName)
{
    m_roomName = roomName;
}

size_t DateEvent::getHash()
{
    size_t sum = 0;
    sum ^= qHash(m_id);
    sum ^= qHash(m_start);
    sum ^= qHash(m_end);
    sum ^= qHash(m_summary);
    sum ^= qHash(m_roomName);
    return sum;
}

QDebug operator<<(QDebug debug, const DateEvent &dateEvent)
{
    QDebugStateSaver saver(debug);
    debug.nospace().noquote() << "DateEvent("
                              << "id=" << dateEvent.id() << ","
                              << "start=" << dateEvent.start() << ","
                              << "end=" << dateEvent.end() << ","
                              << "roomName=" << dateEvent.roomName() << ","
                              << "summary=" << dateEvent.summary() << ","
                              << "source=" << dateEvent.source() << ")";
    return debug;
}
