#include "MatrixChatRoom.h"
#include "ChatMessage.h"

MatrixChatRoom::MatrixChatRoom(const QString &id, const QString &name, QObject *parent)
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
    m_messages.append(chatMessageObject);
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

ChatMessage *MatrixChatRoom::sendMessage(const QString &message)
{
    Q_UNUSED(message)
    // TODO
    return nullptr;
}
