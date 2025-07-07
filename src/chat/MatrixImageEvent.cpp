#include "MatrixImageEvent.h"

MatrixImageEvent::MatrixImageEvent(const QString &eventId, const QString &roomId,
                                   const QString &senderId, const QDateTime &dateTime,
                                   const QString &imageUrl, QObject *parent)
    : MatrixEvent{ eventId, roomId, senderId, dateTime, parent }, m_imageUrl(imageUrl)
{
    qCritical() << "===> 1" << imageUrl << "2" << m_imageUrl;
}
