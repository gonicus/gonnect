#include "JsChatRoom.h"
#include "ChatMessage.h"
#include "JsChatConnector.h"
#include "JsChatEvent.h"

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
    Q_EMIT connectorParent() -> resetUnreadCountRequested(m_id);
}

void JsChatRoom::sendMessage(const QString &message)
{
    Q_EMIT connectorParent() -> sendMessageRequested(m_id, message);
}

JsChatConnector *JsChatRoom::connectorParent() const
{
    return qobject_cast<JsChatConnector *>(parent());
}
