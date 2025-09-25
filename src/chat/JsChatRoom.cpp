#include "JsChatRoom.h"
#include "ChatMessage.h"
#include "JsChatConnector.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcJsChatRoom, "gonnect.chat.JsChatRoom")

JsChatRoom::JsChatRoom(const QString &id, const QString &name, JsChatConnector *parent)
    : IChatRoom{ parent }, m_id{ id }, m_name{ name }
{
}

JsChatRoom::~JsChatRoom()
{
    qDeleteAll(m_messages);
    m_messages.clear();
}

void JsChatRoom::addMessage(ChatMessage *chatMessageObject)
{
    Q_CHECK_PTR(chatMessageObject);
    chatMessageObject->setFlags(chatMessageObject->flags() | ChatMessage::Flag::Markdown);
    m_messages.append(chatMessageObject);
    Q_EMIT chatMessageAdded(m_messages.length() - 1, chatMessageObject);
}

void JsChatRoom::setUnreadNotificationCount(qsizetype count)
{
    m_unreadCount = count;
    Q_EMIT notificationCountChanged(count);
}

void JsChatRoom::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        Q_EMIT nameChanged(name);
    }
}

void JsChatRoom::resetUnreadCount()
{
    Q_EMIT connectorParent()->resetUnreadCountRequested(m_id);
}

void JsChatRoom::sendMessage(const QString &message)
{
    Q_EMIT connectorParent()->sendMessageRequested(m_id, message);
}

JsChatConnector *JsChatRoom::connectorParent() const
{
    return qobject_cast<JsChatConnector *>(parent());
}

ChatMessage *JsChatRoom::chatMessageByEventId(const QString &eventId) const
{
    for (const auto chatMessageObj : std::as_const(m_messages)) {
        if (chatMessageObj->eventId() == eventId) {
            return chatMessageObj;
        }
    }
    return nullptr;
}

void JsChatRoom::toggleReaction(const QString &eventId, const QString &emoji)
{
    const auto chatMessageObj = chatMessageByEventId(eventId);
    if (!chatMessageObj) {
        qCCritical(lcJsChatRoom) << "Unable to find object for event id" << eventId;
        return;
    }

    const auto reactionObj = chatMessageObj->reactionByEmoji(emoji);
    if (reactionObj && !reactionObj->ownReactEventId.isEmpty()) {
        Q_EMIT connectorParent()->redactEventRequested(id(), reactionObj->ownReactEventId);
    } else {
        connectorParent()->reactToMessage(this, eventId, emoji);
    }
}

void JsChatRoom::setReactionCount(const QString &eventId, const QString &emoji, qsizetype count,
                                  const QString &ownReactEventId)
{
    qsizetype idx = 0;
    for (auto chatMessageObj : std::as_const(m_messages)) {
        if (chatMessageObj->eventId() == eventId) {
            chatMessageObj->setReactionCount(emoji, count, ownReactEventId);
            Q_EMIT reactionChanged(eventId, idx);
            return;
        }
        ++idx;
    }

    qCWarning(lcJsChatRoom) << "Cannot find ChatMessage object for event id" << eventId;
}
