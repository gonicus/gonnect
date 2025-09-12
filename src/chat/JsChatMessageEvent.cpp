#include "JsChatMessageEvent.h"

JsChatMessageEvent::JsChatMessageEvent(const QString &eventId, const QString &roomId,
                                       const QString &senderId, const QString &displayName,
                                       const QDateTime &dateTime, const QString &message,
                                       ChatMessage::Flags flags, QObject *parent)
    : JsChatEvent{ eventId, roomId, senderId, dateTime, parent },
      ChatMessage{ eventId, senderId, displayName, message, dateTime, flags }
{
}
