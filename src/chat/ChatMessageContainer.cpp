#include "ChatMessageContainer.h"
#include "IChatRoom.h"
#include "ChatMessageContentVideoFile.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcChatMessageContainer, "gonnect.app.chat.ChatMessageContainer")

ChatMessageContainer::ChatMessageContainer(IChatRoom *parent) : QObject{ parent }
{
    Q_CHECK_PTR(parent);
}

ChatMessageContainer::~ChatMessageContainer()
{
    clear();
}

void ChatMessageContainer::clear()
{
    qDeleteAll(m_messageLookup);
    m_messageLookup.clear();
    m_messages.clear();
}

IChatRoom *ChatMessageContainer::chatRoom() const
{
    return q_check_ptr(qobject_cast<IChatRoom *>(parent()));
}

ChatMessage *ChatMessageContainer::at(qsizetype index) const
{
    if (index < 0 || index >= m_messages.count()) {
        return nullptr;
    }
    return m_messages.at(index);
}

ChatMessage *ChatMessageContainer::messageById(const QString &messageId) const
{
    if (messageId.isEmpty()) {
        return nullptr;
    }
    return m_messageLookup.value(messageId, nullptr);
}

qsizetype ChatMessageContainer::indexOf(const ChatMessage *message) const
{
    if (!message) {
        return -1;
    }

    return m_messages.indexOf(message);
}

bool ChatMessageContainer::contains(const ChatMessage *message) const
{
    if (!message) {
        return false;
    }

    return m_messageLookup.contains(message->eventId());
}

void ChatMessageContainer::addMessage(ChatMessage *message, bool isUnread, bool isIndependent)
{
    if (!message) {
        qCWarning(lcChatMessageContainer) << "Ignoring given nullptr parameter";
        return;
    }

    message->setParent(this);

    if (isUnread) {
        setUnreadCount(unreadCount() + 1);
    }

    if (auto *content = qobject_cast<ChatMessageContentVideoFile *>(message->content())) {
        // Thumbnail path is usually set later
        connect(content, &ChatMessageContentVideoFile::thumbnailFilePathChanged, this,
                [this, message]() {
                    Q_EMIT chatMessageContentChanged(m_messages.indexOf(message), message);
                });
    }

    const auto eventId = message->eventId();
    m_messageLookup.insert(eventId, message);

    if (isIndependent) {
        // Independent messages are added to lookup map but not the chronological timeline
        Q_EMIT chatMessageOutOfSequenceReceived(message);

    } else {

        // Find correct place in the chronological timeline
        for (qsizetype i = m_messages.length() - 1; i >= 0; --i) {
            if (m_messages.at(i)->timestamp() < message->timestamp()) {
                m_messages.insert(i + 1, message);
                Q_EMIT chatMessageAdded(i + 1, message);
                return;
            }
        }

        // Fallback
        m_messages.prepend(message);
        Q_EMIT chatMessageAdded(0, message);
    }
}

ChatMessage *ChatMessageContainer::removeMessage(const QString &messageId)
{
    // Remove message from lists and maps
    if (auto *message = m_messageLookup.take(messageId)) {
        const auto index = m_messages.indexOf(message);
        if (index >= 0) {
            m_messages.removeAt(index);
            Q_EMIT chatMessageRemoved(index, message);
        }
        message->deleteLater();
        return message;
    }

    return nullptr;
}

ChatMessage *ChatMessageContainer::updateMessageEventId(const QString &oldEventId,
                                                        const QString &newEventId)
{
    auto msg = m_messageLookup.take(oldEventId);
    if (!msg) {
        return nullptr;
    }

    msg->setEventId(newEventId);
    m_messageLookup.insert(newEventId, msg);
    return msg;
}

void ChatMessageContainer::setUnreadCount(qsizetype count)
{
    if (m_unreadCount != count) {
        m_unreadCount = count;
        Q_EMIT unreadCountChanged();
    }
}
