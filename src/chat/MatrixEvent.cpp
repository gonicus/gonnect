#include "MatrixEvent.h"

MatrixEvent::MatrixEvent(const QString &eventId, const QString &roomId, const QString &senderId,
                         const QDateTime &dateTime, QObject *parent)
    : QObject(parent),
      m_eventId{ eventId },
      m_roomId{ roomId },
      m_senderId{ senderId },
      m_dateTime{ dateTime }
{
}
