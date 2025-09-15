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

void ChatMessage::setReactionCount(const QString &reaction, qsizetype count, bool hasOwnReaction)
{
    if (count) {

        bool found = false;

        for (auto &entry : m_sortedReactions) {
            if (entry.emoji == reaction) {
                found = true;
                entry.count = count;
                entry.hasOwnReaction = hasOwnReaction;
                break;
            }
        }

        if (!found) {
            m_sortedReactions.append(ChatMessageReaction(reaction, count, hasOwnReaction));
        }

        std::sort(m_sortedReactions.begin(), m_sortedReactions.end(),
                  [](const ChatMessageReaction &a, const ChatMessageReaction &b) {
                      return a.count > b.count;
                  });

    } else {
        QMutableListIterator it(m_sortedReactions);
        while (it.hasNext()) {
            const auto &entry = it.next();
            if (entry.emoji == reaction) {
                it.remove();
                break;
            }
        }
    }
}
