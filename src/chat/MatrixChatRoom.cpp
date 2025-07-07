#include "MatrixChatRoom.h"
#include "ChatMessage.h"
#include "MatrixConnector.h"
#include "MatrixEvent.h"

MatrixChatRoom::MatrixChatRoom(const QString &id, const QString &name, MatrixConnector *parent)
    : IChatRoom{ parent }, m_id{ id }, m_name{ name }
{
}

MatrixChatRoom::~MatrixChatRoom()
{
    qDeleteAll(m_messages);
    m_messages.clear();
}

void MatrixChatRoom::addMessage(ChatMessage *chatMessageObject)
{
    Q_CHECK_PTR(chatMessageObject);
    chatMessageObject->setFlags(chatMessageObject->flags() | ChatMessage::Flag::Markdown);
    m_messages.append(chatMessageObject);
    emit chatMessageAdded(m_messages.length() - 1, chatMessageObject);
}

void MatrixChatRoom::setUnreadNotificationCount(qsizetype count)
{
    m_unreadCount = count;
    emit notificationCountChanged(count);
}

void MatrixChatRoom::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        emit nameChanged(name);
    }
}

void MatrixChatRoom::resetUnreadCount()
{
    MatrixEvent *foundEvent = nullptr;
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

void MatrixChatRoom::sendMessage(const QString &message)
{
    emit connectorParent() -> sendMessageRequested(m_id, message);
}

MatrixConnector *MatrixChatRoom::connectorParent() const
{
    return qobject_cast<MatrixConnector *>(parent());
}
