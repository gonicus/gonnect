#include "DateEvent.h"

#include <QHash>
#include <QUrl>
#include <QRegularExpression>
#include "GlobalInfo.h"

DateEvent::DateEvent(const QString &id, const QString &source, const QDateTime &start,
                     const QDateTime &end, const QString &summary, const QString &location,
                     const QString &description, QObject *parent)
    : QObject{ parent },
      m_id{ id },
      m_source{ source },
      m_start{ start },
      m_end{ end },
      m_summary{ summary },
      m_location{ location },
      m_description{ description }
{
    checkForLinks();
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

void DateEvent::setLocation(const QString &location)
{
    m_location = location;
    checkForLinks();
}

void DateEvent::setDescription(const QString &description)
{
    m_description = description;
    checkForLinks();
}

void DateEvent::setIsJitsiMeeting(bool isJitsiMeeting)
{
    m_isJitsiMeeting = isJitsiMeeting;
}

size_t DateEvent::getHash()
{
    size_t sum = 0;
    sum ^= qHash(m_id);
    sum ^= qHash(m_start);
    sum ^= qHash(m_end);
    sum ^= qHash(m_summary);
    sum ^= qHash(m_location);
    return sum;
}

void DateEvent::checkForLinks()
{
    static const QRegularExpression jitsiRoomRegex(
            QString("%1/(.*)$").arg(QRegularExpression::escape(GlobalInfo::instance().jitsiUrl())),
            QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression teamsRegex(
            QString("<(%1/([^>]*))>")
                    .arg(QRegularExpression::escape(GlobalInfo::instance().teamsUrl())),
            QRegularExpression::CaseInsensitiveOption);

    m_link = "";
    m_roomName = "";

    const auto matchResult = jitsiRoomRegex.match(m_location);
    if (matchResult.hasMatch()) {
        m_roomName = matchResult.captured(1);
        m_isJitsiMeeting = true;
    } else {
        QUrl otherLink = QUrl(m_location, QUrl::StrictMode);
        if (otherLink.isValid() && otherLink.scheme() == "https") {
            m_link = otherLink.toString();
            m_isOtherLink = true;
        }
    }

    // Check for other well known links in the description if nothing is found in the location
    if (m_link.isEmpty()) {
        const auto matchResult = teamsRegex.match(m_description);
        if (matchResult.hasMatch()) {
            m_link = matchResult.captured(1);
            m_isOtherLink = true;
        }
    }
}

QDebug operator<<(QDebug debug, const DateEvent &dateEvent)
{
    QDebugStateSaver saver(debug);
    debug.nospace().noquote() << "DateEvent("
                              << "id=" << dateEvent.id() << ","
                              << "source=" << dateEvent.source() << ","
                              << "start=" << dateEvent.start() << ","
                              << "end=" << dateEvent.end() << ","
                              << "summary=" << dateEvent.summary() << ","
                              << "location=" << dateEvent.location() << ","
                              << "description=" << dateEvent.description() << ","
                              << "roomName=" << dateEvent.roomName() << ","
                              << "link=" << dateEvent.link() << ","
                              << "isJitsiMeeting=" << dateEvent.isJitsiMeeting() << ","
                              << "isOtherLink=" << dateEvent.isOtherLink() << ")";
    return debug;
}
