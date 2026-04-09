#include "RTTMessage.h"

RTTMessage::RTTMessage(qint64 timestamp, const QString &sender, const QString &message,
                       bool isMe, bool isFinished)
    : m_timestamp{ timestamp },
      m_sender{ sender },
      m_message{ message },
      m_isMe{ isMe },
      m_isFinished{ isFinished }
{
}
