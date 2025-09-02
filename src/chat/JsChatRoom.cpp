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
    emit chatMessageAdded(m_messages.length() - 1, chatMessageObject);
}

void JsChatRoom::setUnreadNotificationCount(qsizetype count)
{
    m_unreadCount = count;
    emit notificationCountChanged(count);
}

void JsChatRoom::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        emit nameChanged(name);
    }
}

void JsChatRoom::resetUnreadCount()
{
    JsChatEvent *foundEvent = nullptr;
    auto connector = connectorParent();
    const auto &events = connector->events();

    for (const auto event : events) {
        if (event->roomId() == m_id
            && (!foundEvent || event->dateTime() > foundEvent->dateTime())) {
            foundEvent = event;
        }
    }

    if (foundEvent) {
        emit connector->resetUnreadCountRequested(m_id, foundEvent->eventId());
    }
}

void JsChatRoom::sendMessage(const QString &message)
{
    emit connectorParent() -> sendMessageRequested(m_id, message);
}

JsChatConnector *JsChatRoom::connectorParent() const
{
    return qobject_cast<JsChatConnector *>(parent());
}
