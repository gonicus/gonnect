#include "RTTMessage.h"

RTTMessage::RTTMessage(qint64 timestamp, const QString &message, bool isMe, bool isFinished)
    : m_timestamp{ timestamp }, m_message{ message }, m_isMe{ isMe }, m_isFinished{ isFinished }
{
}
