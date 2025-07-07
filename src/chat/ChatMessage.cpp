#include "ChatMessage.h"

ChatMessage::ChatMessage(const QString &eventId, const QString &fromId, const QString &nickName,
                         const QString &message, const QDateTime &timestamp, Flags flags)
    : m_eventId{ eventId },
      m_fromId{ fromId },
      m_nickName{ nickName },
      m_message{ message },
      m_timestamp{ timestamp },
      m_flags{ flags }
{
}
