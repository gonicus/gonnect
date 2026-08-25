#include "IpcChatRoom.h"
#include "ChatMessage.h"
#include "ChatUser.h"
#include "IpcDispatcher.h"
#include "ChatMessageContentText.h"
#include "ChatMessageContentVideoFile.h"
#include "AddressBook.h"

#include <QFileInfo>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcIpcChatRoom, "gonnect.app.chat.IpcChatRoom")

IpcChatRoom::IpcChatRoom(const QString &id, const QString &name, IChatProvider *chatProvider)
    : IChatRoom{ chatProvider, chatProvider }, m_id{ id }, m_name{ name }
{
    connect(this, &IpcChatRoom::chatUsersChanged, this, &IpcChatRoom::updateIsDirectChat);
    connect(this, &IpcChatRoom::otherUserChanged, this, &IpcChatRoom::avatarPathChanged);
    connect(this, &IpcChatRoom::otherUserChanged, this, &IpcChatRoom::hasPresenceState);
    connect(this, &IpcChatRoom::otherUserChanged, this, [this]() { presenceState(); });
    connect(this, &IpcChatRoom::chatUserRoomStateChanged, this,
            &IpcChatRoom::updateOwnUserJoinState);

    updateIsDirectChat();
}

IpcChatRoom::~IpcChatRoom()
{
    qDeleteAll(m_messageLookup);
    m_messageLookup.clear();
}

void IpcChatRoom::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        Q_EMIT nameChanged(name);
    }
}

void IpcChatRoom::setAvatarPath(const QString &path)
{
    if (m_avatarPath != path) {
        m_avatarPath = path;
        Q_EMIT avatarPathChanged();
    }
}

void IpcChatRoom::setInvitationText(const QString &invitationText)
{
    if (m_invitationText != invitationText) {
        m_invitationText = invitationText;
        Q_EMIT invitationTextChanged();
    }
}

void IpcChatRoom::setJoinRule(JoinRule joinRule)
{
    if (m_joinRule != joinRule) {
        m_joinRule = joinRule;
        Q_EMIT joinRuleChanged();
    }
}

void IpcChatRoom::setIsDirect(bool value)
{
    if (m_isDirectChat != value) {
        m_isDirectChat = value;
        Q_EMIT isDirectChatChanged();
    }
}

void IpcChatRoom::resetUnreadCount()
{
    if (m_unreadCount) {
        ipcDispatcher()->markAsRead(id());
        setUnreadCount(0);
    }
}

ChatMessage *IpcChatRoom::pinnedChatMessageByIndex(qsizetype index) const
{
    if (index < 0 || index >= m_pinnedMessages.size()) {
        qCWarning(lcIpcChatRoom) << "Invalid list index:" << index;
        return nullptr;
    }
    return m_pinnedMessages.at(index);
}

ChatMessage *IpcChatRoom::chatMessageById(const QString &id) const
{
    if (id.isEmpty()) {
        return nullptr;
    }

    return m_messageLookup.value(id, nullptr);
}

qsizetype IpcChatRoom::indexOfPinnedChatMessage(ChatMessage *message) const
{
    if (!message) {
        qCWarning(lcIpcChatRoom) << "Cannot give index of nullptr";
        return -1;
    }

    return m_pinnedMessages.indexOf(message);
}

void IpcChatRoom::ensureMessageLoaded(const QString &id)
{
    if (id.isEmpty() || m_messageLookup.contains(id) || m_loadRequestedMessageIds.contains(id)) {
        return;
    }

    if (auto *dispatcher = ipcDispatcher()) {
        m_loadRequestedMessageIds.insert(id);
        dispatcher->loadSingleMessage(m_id, id);
    }
}

ChatMessage *IpcChatRoom::latestOwnTextMessage() const
{
    using Flag = ChatMessage::Flag;

    QListIterator it(m_messages);
    it.toBack();

    while (it.hasPrevious()) {
        auto *msg = it.previous();
        if ((msg->flags() & Flag::OwnMessage)
            && qobject_cast<ChatMessageContentText *>(msg->content())
            && !(msg->flags() & (Flag::Pending | Flag::Failed))) {
            return msg;
        }
    }

    return nullptr;
}

void IpcChatRoom::sendMessage(const QString &message, const QString &relatedMessageId)
{
    ipcDispatcher()->sendMessage(id(), message, relatedMessageId);
}

void IpcChatRoom::sendFile(const QString &filePath)
{
    auto dispatcher = ipcDispatcher();
    const auto uploadedUrl = dispatcher->uploadFile(filePath);
    if (uploadedUrl.isEmpty()) {
        qCCritical(lcIpcChatRoom) << "Error on uploading file" << filePath;
        return;
    }

    const QUrl url(filePath);
    const auto originalFileName = url.isLocalFile() ? QFileInfo(url.toLocalFile()).fileName()
                                                    : filePath.split(QChar('/')).last();
    dispatcher->sendFile(id(), uploadedUrl, originalFileName);
}

void IpcChatRoom::sendTypingPing()
{
    if (auto *dispatcher = ipcDispatcher()) {
        dispatcher->sendTypingPing(id());
    } else {
        qCCritical(lcIpcChatRoom) << "IpcChatRoom has no IpcDispatcher as parent";
    }
}

void IpcChatRoom::togglePin(const QString &messageId)
{
    if (auto *dispatcher = ipcDispatcher()) {
        const bool isCurrentlyPinned = m_pinnedMessageIds.contains(messageId);
        dispatcher->pinOrUnpinMessage(id(), messageId, !isCurrentlyPinned);
    } else {
        qCCritical(lcIpcChatRoom) << "IpcChatRoom has no IpcDispatcher as parent";
    }
}

void IpcChatRoom::addExistingMessage(ChatMessage *message, bool isUnread, bool isIndependent)
{
    Q_CHECK_PTR(message);

    if (isUnread) {
        setUnreadCount(notificationCount() + 1);
    }

    if (auto *content = qobject_cast<ChatMessageContentVideoFile *>(message->content())) {
        // Since the thumbnail path is available later, it must produce a signal
        connect(content, &ChatMessageContentVideoFile::thumbnailFilePathChanged, this,
                [this, message]() {
                    Q_EMIT chatMessageContentChanged(indexOfMessage(message), message);
                });
    }

    const auto eventId = message->eventId();
    m_loadRequestedMessageIds.remove(eventId);

    if (isIndependent) {
        m_messageLookup.insert(eventId, message);
        updatePinnedMessages();
        Q_EMIT chatMessageOutOfSequenceReceived(message);

    } else {
        for (qsizetype i = m_messages.length() - 1; i >= 0; --i) {
            if (m_messages.at(i)->timestamp() < message->timestamp()) {
                m_messages.insert(i + 1, message);
                m_messageLookup.insert(eventId, message);
                updatePinnedMessages();
                Q_EMIT chatMessageAdded(i + 1, message);

                if (m_lastReadUsers.contains(eventId)) {
                    Q_EMIT lastMessageReadChanged(indexOfMessage(message), message);
                }
                return;
            }
        }

        m_messages.prepend(message);
        m_messageLookup.insert(message->eventId(), message);
        updatePinnedMessages();
        Q_EMIT chatMessageAdded(0, message);
    }

    if (m_lastReadUsers.contains(eventId)) {
        Q_EMIT lastMessageReadChanged(indexOfMessage(message), message);
    }
}

qsizetype IpcChatRoom::indexOfMessage(const ChatMessage *message) const
{
    return m_messages.indexOf(message);
}

void IpcChatRoom::removeMessage(const QString &messageId)
{
    m_messageLookup.remove(messageId);

    for (qsizetype i = m_messages.length() - 1; i >= 0; --i) {
        if (m_messages.at(i)->eventId() == messageId) {
            auto message = m_messages.at(i);
            m_pinnedMessageIds.removeOne(messageId);
            if (m_pinnedMessages.removeOne(message)) {
                Q_EMIT pinnedMessagesChanged();
            }
            m_messages.removeAt(i);
            Q_EMIT chatMessageRemoved(i, message);
            delete message;
            return;
        }
    }
}

void IpcChatRoom::setPinnedMessageIds(const QStringList &messageIds)
{
    if (m_pinnedMessageIds != messageIds) {
        m_pinnedMessageIds = messageIds;
        updatePinnedMessages();
    }
}

void IpcChatRoom::updateMessageEventId(const QString &oldEventId, const QString &newEventId)
{
    if (auto msg = m_messageLookup.take(oldEventId)) {
        msg->setEventId(newEventId);
        m_messageLookup.insert(newEventId, msg);

        if (const auto users = m_lastReadUsers.take(oldEventId); !users.isEmpty()) {
            m_lastReadUsers.insert(newEventId, users);
            for (const auto &uid : users) {
                if (m_lastReadMessage.value(uid) == oldEventId) {
                    m_lastReadMessage.insert(uid, newEventId);
                }
            }
            Q_EMIT lastMessageReadChanged(indexOfMessage(msg), msg);
        }

        Q_EMIT chatMessageEventIdChanged(indexOfMessage(msg), msg);
    }
}

void IpcChatRoom::setMessageFlags(const QString &eventId, ChatMessage::Flags newFlags)
{
    if (auto msg = m_messageLookup.value(eventId)) {
        const auto prevFlags = msg->flags();
        if (prevFlags != newFlags) {
            msg->setFlags(newFlags);
            Q_EMIT chatMessageFlagsChanged(indexOfMessage(msg), msg, prevFlags);
        }
    }
}

bool IpcChatRoom::hasPresenceState()
{
    if (!isDirectChat()) {
        return false;
    }

    if (const auto other = otherUser()) {
        return other->hasPresenceState();
    }

    return false;
}

ChatUser::PresenceState IpcChatRoom::presenceState() const
{
    if (m_isDirectChat) {
        if (const auto other = otherUser(); other && other->hasPresenceState()) {
            return other->presenceState();
        }
    }

    return ChatUser::PresenceState::Unknown;
}

IChatRoom::UserRoomState IpcChatRoom::ownUserJoinState() const
{
    return m_ownUserJoinState;
}

ChatUser *IpcChatRoom::otherUser() const
{
    return m_otherUser;
}

void IpcChatRoom::setUnreadCount(qsizetype count)
{
    if (m_unreadCount != count) {
        m_unreadCount = count;
        Q_EMIT notificationCountChanged(count);
    }
}

void IpcChatRoom::setPermissions(Permissions permissions)
{
    if (m_permissions != permissions) {
        m_permissions = permissions;
        Q_EMIT permissionsChanged();
    }
}

void IpcChatRoom::setIsFavorite(bool value)
{
    if (m_isFavorite != value) {
        m_isFavorite = value;
        Q_EMIT isFavoriteChanged();
    }
}

QString IpcChatRoom::name()
{
    if (!m_name.isEmpty()) {
        return m_name;
    }
    if (const auto *other = otherUser()) {
        return other->computedName();
    }
    return QString();
}

QString IpcChatRoom::avatarPath()
{
    if (!m_avatarPath.isEmpty()) {
        return m_avatarPath;
    }
    if (const auto *other = otherUser()) {
        if (const auto *contact = AddressBook::instance().lookupByChatUser(other)) {
            return contact->avatarPath();
        }
        return other->avatarPath();
    }
    return "";
}

void IpcChatRoom::loadMessages()
{
    if (isLoadingMessageHistory() || isCompletelyLoaded()) {
        return;
    }

    ipcDispatcher()->loadMessages(this);

    if (!m_isInitiallyLoaded) {
        m_isInitiallyLoaded = true;
        Q_EMIT IChatRoom::isInitiallyLoadedChanged();
    }
}

void IpcChatRoom::addUser(ChatUser *user, UserRoomState state)
{
    if (!user) {
        qCCritical(lcIpcChatRoom) << "The user must not be nullptr - ignoring";
        return;
    }

    if (m_chatUsers.contains(user)) {
        qCWarning(lcIpcChatRoom) << "The user is already contained in this room - ignoring";
        return;
    }

    qsizetype idx = -1;
    for (qsizetype i = 0; i < m_chatUsers.length(); ++i) {
        if (m_chatUsers.at(i)->computedName().localeAwareCompare(user->computedName()) > 0) {
            idx = i;
            break;
        }
    }

    if (idx < 0) {
        idx = std::max(static_cast<qsizetype>(0), m_chatUsers.length());
    }

    connect(user, &ChatUser::destroyed, this, [this](QObject *obj) {
        if (auto user = qobject_cast<ChatUser *>(obj)) {
            removeUser(user);
        }
    });

    connect(user, &ChatUser::avatarPathChanged, this, [this]() {
        if (m_isDirectChat) {
            Q_EMIT avatarPathChanged();
        }
    });

    m_chatUsers.insert(idx, user);
    m_chatUserLookup.insert(user->id(), user);
    m_userRoomStates.insert(user, state);

    Q_EMIT chatUserAdded(idx, user, state);
    Q_EMIT chatUsersChanged();
    Q_EMIT chatUserRoomStateChanged(idx, user, state);

    if (const auto &messageId = m_lastReadMessage.value(user->id(), QString());
        !messageId.isEmpty()) {
        if (auto *message = m_messageLookup.value(messageId, nullptr)) {
            if (const auto index = m_messages.indexOf(message); index >= 0) {
                Q_EMIT lastMessageReadChanged(index, message);
            }
        }
    }

    updateOtherUser();
}

void IpcChatRoom::removeUser(ChatUser *user)
{
    if (!user) {
        qCCritical(lcIpcChatRoom) << "The user must not be nullptr - ignoring";
        return;
    }

    const auto idx = m_chatUsers.indexOf(user);

    if (idx >= 0) {
        m_chatUsers.removeAt(idx);
        m_chatUserLookup.remove(user->id());
        m_userRoomStates.remove(user);

        user->disconnect(this);

        Q_EMIT chatUserRemoved(idx, user);
        Q_EMIT chatUsersChanged();
        updateOtherUser();
    } else {
        qCCritical(lcIpcChatRoom) << "The user" << *user << "is supposed to be removed from room"
                                  << id() << "but could not be found amongst its members.";
    }
}

void IpcChatRoom::setUserRoomState(ChatUser *user, UserRoomState state)
{
    if (!user) {
        qCCritical(lcIpcChatRoom) << "The user must not be nullptr - ignoring";
        return;
    }

    if (!m_chatUsers.contains(user)) {
        qCCritical(lcIpcChatRoom) << "The user" << *user << "is supposed to be updated in room"
                                  << id() << "but could not be found amongst its members.";
        return;
    }

    m_userRoomStates.insert(user, state);
    Q_EMIT chatUserRoomStateChanged(m_chatUsers.indexOf(user), user, state);
}

void IpcChatRoom::setUserRoomState(qsizetype index, UserRoomState state)
{
    if (index >= m_chatUsers.length()) {
        qCCritical(lcIpcChatRoom)
                << QString("The index %1 is out of bounds (number of users is %2)")
                           .arg(index)
                           .arg(m_chatUsers.length());
        return;
    }

    auto user = q_check_ptr(m_chatUsers.at(index));

    m_userRoomStates.insert(user, state);
    Q_EMIT chatUserRoomStateChanged(index, user, state);
}

ChatUser *IpcChatRoom::chatUserById(const QString &userId) const
{
    return m_chatUserLookup.value(userId, nullptr);
}

void IpcChatRoom::setTypingUsers(const QList<ChatUser *> &users)
{
    m_typingUsers = users;

    std::sort(m_typingUsers.begin(), m_typingUsers.end(),
              [](const ChatUser *left, const ChatUser *right) -> bool {
                  return left->computedName().localeAwareCompare(right->computedName()) < 0;
              });

    Q_EMIT typingUsersChanged();
}

qsizetype IpcChatRoom::joinedChatUserCount() const
{
    qsizetype count = 0;
    for (auto *user : m_chatUsers) {
        if (m_userRoomStates.value(user) == UserRoomState::Joined) {
            ++count;
        }
    }
    return count;
}

IChatRoom::UserRoomState IpcChatRoom::chatUserRoomState(ChatUser *user) const
{
    if (!user) {
        qCCritical(lcIpcChatRoom) << "The user must not be nullptr - ignoring";
        return UserRoomState::Unjoined;
    }

    if (!m_userRoomStates.contains(user)) {
        return UserRoomState::Unjoined;
    }

    return m_userRoomStates.value(user);
}

bool IpcChatRoom::isUserMemberOfRoom(const QString &userId) const
{
    const auto user = m_chatUserLookup.value(userId, nullptr);
    static const QSet<UserRoomState> okStates = { UserRoomState::Joined, UserRoomState::Invited,
                                                  UserRoomState::Knocked };
    return user && okStates.contains(m_userRoomStates.value(user));
}

bool IpcChatRoom::isUserInvitable(ChatUser *user) const
{
    if (!user) {
        return false;
    }

    static const QSet<UserRoomState> allowedStates = { UserRoomState::Unjoined,
                                                       UserRoomState::Banned };

    return allowedStates.contains(chatUserRoomState(user));
}

const QList<ChatUser *> &IpcChatRoom::chatUsers() const
{
    return m_chatUsers;
}

const QList<ChatUser *> &IpcChatRoom::typingUsers() const
{
    return m_typingUsers;
}

QList<ChatUser *> IpcChatRoom::lastMessageRead(const QString &messageId) const
{
    QList<ChatUser *> l;
    if (messageId.isEmpty() || !m_lastReadUsers.contains(messageId)) {
        return l;
    }

    // Fill list with real objects
    const auto &userIds = m_lastReadUsers[messageId];
    l.reserve(userIds.size());

    for (const auto &userId : userIds) {
        if (auto *user = m_chatUserLookup.value(userId, nullptr)) {
            l.append(user);
        } else {
            qCWarning(lcIpcChatRoom) << "Cannot resolve user" << userId << "of read marker"
                                     << messageId << "in room" << m_id;
        }
    }

    return l;
}

void IpcChatRoom::setLastMessageRead(const QString &userId, const QString &messageId)
{
    if (userId.isEmpty() || messageId.isEmpty()) {
        qCWarning(lcIpcChatRoom) << "Both userId and messageId must not be empty";
        return;
    }

    // Remove old entry
    const auto &oldMessageId = m_lastReadMessage.value(userId, QString());
    if (!oldMessageId.isEmpty()) {
        auto it = m_lastReadUsers.find(oldMessageId);
        if (it != m_lastReadUsers.end()) {
            it->remove(userId);
            if (it->isEmpty()) {
                m_lastReadUsers.erase(it);
            }

            if (auto *oldMessage = m_messageLookup.value(oldMessageId, nullptr)) {
                Q_EMIT lastMessageReadChanged(m_messages.indexOf(oldMessage), oldMessage);
            }
        }
    }

    // Insert new entry
    m_lastReadMessage.insert(userId, messageId);
    m_lastReadUsers[messageId].insert(userId);

    // Message might not exist yet - especially on initial loading
    if (auto *message = m_messageLookup.value(messageId, nullptr)) {
        Q_EMIT lastMessageReadChanged(m_messages.indexOf(message), message);
    }
}

void IpcChatRoom::clear()
{
    m_pinnedMessageIds.clear();
    m_loadRequestedMessageIds.clear();
    m_lastReadMessage.clear();
    m_lastReadUsers.clear();

    if (!m_pinnedMessages.isEmpty()) {
        m_pinnedMessages.clear();
        Q_EMIT pinnedMessagesChanged();
    }

    m_messageLookup.clear();
    qDeleteAll(m_messages);
    m_messages.clear();

    Q_EMIT chatMessagesReset();
}

void IpcChatRoom::updateIsDirectChat()
{
    setIsDirect(m_chatUsers.size() <= 2);
}

void IpcChatRoom::updateOtherUser()
{
    // Find other
    ChatUser *other = nullptr;
    if (m_chatUsers.size() == 2) {
        const auto chatProvider = q_check_ptr(qobject_cast<const IpcDispatcher *>(parent()));

        if (m_chatUsers.at(0)->id() == chatProvider->ownUserId()) {
            other = m_chatUsers.at(1);
        } else {
            other = m_chatUsers.at(0);
        }
    }

    // Update
    if (m_otherUser != other) {

        if (m_otherUserContext) {
            m_otherUserContext->deleteLater();
            m_otherUserContext = nullptr;
        }

        m_otherUser = other;

        if (other) {
            m_otherUserContext = new QObject(this);

            connect(other, &QObject::destroyed, m_otherUserContext, [this](QObject *) {
                m_otherUserContext->deleteLater();
                m_otherUserContext = nullptr;
                m_otherUser = nullptr;

                Q_EMIT otherUserChanged();
            });

            connect(other, &ChatUser::displayNameChanged, m_otherUserContext,
                    [this]() { Q_EMIT nameChanged(name()); });

            connect(other, &ChatUser::hasPresenceStateChanged, m_otherUserContext,
                    [this]() { Q_EMIT hasPresenceStateChanged(); });

            connect(other, &ChatUser::presenceStateChanged, m_otherUserContext,
                    [this]() { Q_EMIT presenceStateChanged(); });

            connect(other, &ChatUser::avatarPathChanged, m_otherUserContext,
                    [this]() { Q_EMIT avatarPathChanged(); });

            connect(&AddressBook::instance(), &AddressBook::chatUserMappingAdded,
                    m_otherUserContext, [this](ChatUser *chatUser) {
                        if (chatUser == m_otherUser) {
                            Q_EMIT avatarPathChanged();
                        }
                    });

            connect(&AddressBook::instance(), &AddressBook::chatUserAvatarChanged,
                    m_otherUserContext, [this](ChatUser *chatUser) {
                        if (chatUser == m_otherUser) {
                            Q_EMIT avatarPathChanged();
                        }
                    });
        }

        Q_EMIT otherUserChanged();
        Q_EMIT nameChanged(name());
    }
}

void IpcChatRoom::updateOwnUserJoinState(qsizetype, ChatUser *user, UserRoomState state)
{
    const auto ownUserId = ipcDispatcher()->ownUserId();
    if (user->id() == ownUserId) {
        m_ownUserJoinState = state;
        Q_EMIT ownUserJoinStateChanged();
    }
}

IpcDispatcher *IpcChatRoom::ipcDispatcher() const
{
    return q_check_ptr(qobject_cast<IpcDispatcher *>(parent()));
}

void IpcChatRoom::updatePinnedMessages()
{
    QList<ChatMessage *> messages;
    messages.reserve(m_pinnedMessageIds.size());

    for (const auto &id : std::as_const(m_pinnedMessageIds)) {
        if (auto *chatMessage = chatMessageById(id)) {
            messages.append(chatMessage);
        } else {
            ensureMessageLoaded(id);
        }
    }

    std::ranges::sort(messages, [](const ChatMessage *a, const ChatMessage *b) -> bool {
        return a->timestamp() < b->timestamp();
    });

    if (m_pinnedMessages != messages) {
        m_pinnedMessages = messages;
        Q_EMIT pinnedMessagesChanged();
    }
}
