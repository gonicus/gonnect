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
        connect(content, &ChatMessageContentVideoFile::thumbnailFilePathChanged, message,
                [this, message]() {
                    const auto index = m_messages.indexOf(message);
                    if (index >= 0) {
                        Q_EMIT chatMessageContentChanged(index, message);
                    }
                });
    }

    const auto eventId = message->eventId();
    m_messageLookup.insert(eventId, message);

    if (isIndependent) {
        // Independent messages are added to lookup map but not the chronological timeline
        Q_EMIT chatMessageOutOfSequenceReceived(message);

    } else {

        // Find correct place in the chronological timeline
        auto it = std::ranges::upper_bound(
                m_messages, message, [](const auto &newMsg, const auto &existingMsg) {
                    return newMsg->timestamp() < existingMsg->timestamp();
                });

        const qsizetype index = std::distance(m_messages.begin(), it);
        m_messages.insert(index, message);
        Q_EMIT chatMessageAdded(index, message);
    }
}

bool ChatMessageContainer::removeMessage(const QString &messageId)
{
    // Remove message from lists and maps
    if (auto *message = m_messageLookup.take(messageId)) {
        const auto index = m_messages.indexOf(message);
        if (index >= 0) {
            m_messages.removeAt(index);
            Q_EMIT chatMessageRemoved(index, message);
        }
        message->deleteLater();
        return true;
    }

    return false;
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
