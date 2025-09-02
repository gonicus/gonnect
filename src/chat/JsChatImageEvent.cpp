#include "JsChatImageEvent.h"

JsChatImageEvent::JsChatImageEvent(const QString &eventId, const QString &roomId,
                                   const QString &senderId, const QDateTime &dateTime,
                                   const QString &imageUrl, QObject *parent)
    : JsChatEvent{ eventId, roomId, senderId, dateTime, parent }, m_imageUrl(imageUrl)
{
    qCritical() << "===> 1" << imageUrl << "2" << m_imageUrl;
}
