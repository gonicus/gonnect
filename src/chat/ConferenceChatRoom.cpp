#include "ConferenceChatRoom.h"
#include "ChatMessage.h"
#include "ChatMessageContentText.h"
#include "IConferenceConnector.h"

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

void ConferenceChatRoom::sendMessage(const QString &message, const QString &)
{
    auto confConn = qobject_cast<IConferenceConnector *>(parent());
    const QString ownName = confConn ? confConn->ownDisplayName() : "";

    auto textContent = new ChatMessageContentText(message);
    auto chatMessageObject =
            new ChatMessage("", "", ownName, textContent, QDateTime::currentDateTime(), this,
                            ChatMessage::Flag::OwnMessage);
    addMessage(chatMessageObject);
    Q_EMIT sendMessageRequested(message);
}

ChatUser::PresenceState ConferenceChatRoom::presenceState() const
{
    return ChatUser::PresenceState::Unknown;
}

IChatRoom::UserRoomState ConferenceChatRoom::ownUserJoinState() const
{
    return IChatRoom::UserRoomState::Joined;
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

IChatRoom::JoinRule ConferenceChatRoom::joinRule()
{
    return IChatRoom::JoinRule::Unknown;
}

IChatRoom::Permissions ConferenceChatRoom::permissions()
{
    return IChatRoom::Permissions();
}

ChatMessage *ConferenceChatRoom::chatMessageById(const QString &id) const
{
    Q_UNUSED(id)
    return nullptr;
}

void ConferenceChatRoom::addUser(ChatUser *user, UserRoomState state)
{
    Q_UNUSED(user)
    Q_UNUSED(state)

    // TODO
}

void ConferenceChatRoom::removeUser(ChatUser *user)
{
    Q_UNUSED(user)

    // TODO
}

void ConferenceChatRoom::setUserRoomState(ChatUser *user, UserRoomState state)
{
    Q_UNUSED(user)
    Q_UNUSED(state)

    // TODO
}

void ConferenceChatRoom::setUserRoomState(qsizetype index, UserRoomState state)
{
    Q_UNUSED(index)
    Q_UNUSED(state)

    // TODO
}

ChatUser *ConferenceChatRoom::chatUserById(const QString &userId) const
{
    Q_UNUSED(userId)

    // TODO
    return nullptr;
}

ConferenceChatRoom::UserRoomState
ConferenceChatRoom::chatUserRoomState(ChatUser *user) const
{
    Q_UNUSED(user)

    // TODO

    return UserRoomState::Unjoined;
}

static QList<ChatUser *> dummyList = {};

const QList<ChatUser *> &ConferenceChatRoom::chatUsers() const
{
    // TODO
    return dummyList;
}

const QList<ChatUser *> &ConferenceChatRoom::typingUsers() const
{
    // TODO
    return dummyList;
}

bool ConferenceChatRoom::isUserMemberOfRoom(const QString &userId) const
{
    // TODO
    Q_UNUSED(userId)
    return false;
}

void ConferenceChatRoom::clear()
{
    qDeleteAll(m_messages);
    m_messages.clear();
    Q_EMIT chatMessagesReset();
}
