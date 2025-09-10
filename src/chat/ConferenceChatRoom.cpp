#include "ConferenceChatRoom.h"
#include "ChatMessage.h"

ConferenceChatRoom::ConferenceChatRoom(QObject *parent) : IChatRoom{ parent } { }

ConferenceChatRoom::ConferenceChatRoom(const QString &roomId, const QString &name, QObject *parent)
    : IChatRoom{ parent }, m_id{ roomId }, m_name{ name }
{
}

ConferenceChatRoom::~ConferenceChatRoom()
{
    qDeleteAll(m_messages);
}

qsizetype ConferenceChatRoom::notificationCount()
{
    return 0; // TODO
}

void ConferenceChatRoom::sendMessage(const QString &message)
{
    auto chatMessageObject = new ChatMessage("", "", "", message, QDateTime::currentDateTime(),
                                             ChatMessage::Flag::OwnMessage);
    addMessage(chatMessageObject);
    Q_EMIT sendMessageRequested(message);
}

void ConferenceChatRoom::addMessage(ChatMessage *chatMessageObj)
{
    Q_CHECK_PTR(chatMessageObj);
    m_messages.append(chatMessageObj);
    Q_EMIT chatMessageAdded(m_messages.length() - 1, chatMessageObj);
}

void ConferenceChatRoom::resetUnreadCount()
{
    // Unsupported
}
