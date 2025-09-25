#include "ConferenceChatRoom.h"
#include "ChatMessage.h"
#include "IConferenceConnector.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcConferenceChatRoom, "gonnect.app.conference.ChatRoom")

ConferenceChatRoom::ConferenceChatRoom(IConferenceConnector *parent) : IChatRoom{ parent } { }

ConferenceChatRoom::ConferenceChatRoom(const QString &roomId, const QString &name,
                                       IConferenceConnector *parent)
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
    auto confConn = qobject_cast<IConferenceConnector *>(parent());
    const QString ownName = confConn ? confConn->ownDisplayName() : "";

    auto chatMessageObject = new ChatMessage("", "", ownName, message, QDateTime::currentDateTime(),
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

void ConferenceChatRoom::toggleReaction(const QString &, const QString &)
{
    qCWarning(lcConferenceChatRoom) << "ConferenceChatRoom does not support toggleReaction";
}

void ConferenceChatRoom::setReactionCount(const QString &, const QString &, qsizetype,
                                          const QString &)
{
    qCWarning(lcConferenceChatRoom) << "ConferenceChatRoom does not support setReactionCount";
}
