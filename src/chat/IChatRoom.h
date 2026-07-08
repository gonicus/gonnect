#pragma once

#include <QObject>
#include <QDateTime>
#include <qqmlregistration.h>

#include "ChatUser.h"
#include "NotificationSetting.h"
#include "ChatMessage.h"

struct RoomSettings
{
    NotificationSetting::Setting notificationSetting = NotificationSetting::Setting::None;

    auto operator<=>(const RoomSettings &) const = default;
};

class IChatRoom : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by C++")
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

    Q_PROPERTY(QString id READ id CONSTANT FINAL)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged FINAL)
    Q_PROPERTY(QString avatarPath READ avatarPath NOTIFY avatarPathChanged FINAL)
    Q_PROPERTY(bool isInitiallyLoaded READ isInitiallyLoaded NOTIFY isInitiallyLoadedChanged FINAL)
    Q_PROPERTY(bool isDirectChat READ isDirectChat NOTIFY isDirectChatChanged FINAL)
    Q_PROPERTY(bool isFavorite READ isFavorite NOTIFY isFavoriteChanged FINAL)
    Q_PROPERTY(bool isLoadingMessageHistory READ isLoadingMessageHistory NOTIFY
                       isLoadingMessageHistoryChanged FINAL)
    Q_PROPERTY(
            bool isCompletelyLoaded READ isCompletelyLoaded NOTIFY isCompletelyLoadedChanged FINAL)
    Q_PROPERTY(bool hasPresenceState READ hasPresenceState NOTIFY hasPresenceStateChanged FINAL)
    Q_PROPERTY(ChatUser::PresenceState presenceState READ presenceState NOTIFY presenceStateChanged
                       FINAL)
    Q_PROPERTY(IChatRoom::UserRoomState ownUserJoinState READ ownUserJoinState NOTIFY
                       ownUserJoinStateChanged FINAL)
    Q_PROPERTY(IChatRoom::JoinRule joinRule READ joinRule NOTIFY joinRuleChanged FINAL)
    Q_PROPERTY(IChatRoom::Permissions permissions READ permissions NOTIFY permissionsChanged FINAL)
    Q_PROPERTY(QDateTime latestMessageDateTime READ latestMessageDateTime NOTIFY
                       latestMessageDateTimeChanged FINAL)
    Q_PROPERTY(QList<ChatUser *> chatUsers READ chatUsers NOTIFY chatUsersChanged FINAL)
    Q_PROPERTY(QList<ChatUser *> typingUsers READ typingUsers NOTIFY typingUsersChanged FINAL)
    Q_PROPERTY(qsizetype chatUserCount READ chatUserCount NOTIFY chatUsersChanged FINAL)
    Q_PROPERTY(qsizetype notificationCount READ notificationCount NOTIFY notificationCountChanged
                       FINAL)

public:
    enum class UserRoomState { Unjoined, Joined, Invited, Knocked, Banned };
    Q_ENUM(UserRoomState)

    enum class JoinRule { Unknown, Invite, Knock, Public };
    Q_ENUM(JoinRule)

    enum class LeaveReason { Unknown, User, Kicked, Banned };
    Q_ENUM(LeaveReason)

    enum class Permission {
        CanEdit = 1 << 0,
        CanInvite = 1 << 1,
        CanKick = 1 << 2,
        CanBan = 1 << 3
    };
    Q_ENUM(Permission)
    Q_DECLARE_FLAGS(Permissions, Permission)
    Q_FLAG(Permissions)

    explicit IChatRoom(QObject *parent = nullptr);
    virtual ~IChatRoom() { }

    virtual QString id() = 0;
    virtual QString name() = 0;
    virtual QString avatarPath() = 0;
    virtual bool isFavorite() = 0;
    virtual IChatRoom::JoinRule joinRule() = 0;
    virtual qsizetype notificationCount() = 0;
    virtual IChatRoom::Permissions permissions() = 0;
    Q_INVOKABLE virtual void resetUnreadCount() = 0;

    bool isLoadingMessageHistory() const { return m_isLoadingMessageHistory; }
    void setIsLoadingMessageHistory(bool value);

    bool isCompletelyLoaded() const { return m_isCompletelyLoaded; }
    void setIsCompletelyLoaded(bool value);

    QDateTime latestMessageDateTime() const { return m_latestMessageDateTime; };
    void setLatestMessageDateTime(const QDateTime &dateTime);

    RoomSettings roomSettings() const { return m_roomSettings; }
    void setRoomSettings(const RoomSettings &roomSettings);

    /// List of chat messages of this room, sorted by timestamp ascending
    virtual QList<ChatMessage *> chatMessages() const = 0;

    /// Retrieve a specific message by its id or nullptr, if not found.
    Q_INVOKABLE virtual ChatMessage *chatMessageById(const QString &id) const = 0;

    /// Find the latest text message that was send by the user themself.
    Q_INVOKABLE virtual ChatMessage *latestOwnTextMessage() const = 0;

    /// Send a message in this room.
    Q_INVOKABLE virtual void sendMessage(const QString &message,
                                         const QString &relatedMessageId = "") = 0;

    /// Given a local file url, send a message with this file as an attachment.
    Q_INVOKABLE virtual void sendFile(const QString &filePath) = 0;

    /// Send that the user is currently typing. Shall be called every 2 seconds as long as the user
    /// is typing.
    Q_INVOKABLE virtual void sendTypingPing() = 0;

    /// If the messages of this room have been loaded initially.
    virtual bool isInitiallyLoaded() const = 0;

    /// Start the loading of next batch of messages.
    Q_INVOKABLE virtual void loadMessages() = 0;

    /// Whether this room is a direct chat between two users or a room with several ones.
    virtual bool isDirectChat() = 0;

    /// Whether this room carries a presence state. If it does, the state can be retrieved via
    /// presenceState().
    virtual bool hasPresenceState() = 0;

    /// The current presence state of the room. The result is only valid if hasPresenceState()
    /// returns true.
    virtual ChatUser::PresenceState presenceState() const = 0;

    /// The current join state in this room of the own user.
    virtual IChatRoom::UserRoomState ownUserJoinState() const = 0;

    /// If this is a direct chat, this method must return the other user. If it is not, it
    /// must return nullptr;
    virtual ChatUser *otherUser() const = 0;

    /// The number of current users of this room. Any change in this count must be linked to
    /// the chatUsersChanged() and therefore chatUserAdded() or
    /// chatUserRemoved() signals.
    virtual qsizetype chatUserCount() const = 0;

    /// Add the user object to the room with the given state. This does not invoke any change
    /// on the backend; it just informs about this exisiting user to be a member of the room.
    /// If the user has not been a member before, the according signal chatUserAdded
    /// is sent. If they was a member before, it is ignored. The membership of the user
    /// remains with the caller.
    virtual void addUser(ChatUser *user, UserRoomState state) = 0;

    /// Remove this user object from the room. This does not invoke any action in the
    /// backend; it just applies to the model. If the user is contained in the model, the
    /// according signal chatUserRemoved is sent; the call is ignored otherwise. The
    /// ownership remains with the caller; the object is not destroyed.
    virtual void removeUser(ChatUser *user) = 0;

    /// Update the UserRoomState for the user. If the user is unknown to this room
    /// object, the method must fail silently. If successful, the chatUserRoomStateChanged
    /// signal is emitted.
    virtual void setUserRoomState(ChatUser *user, UserRoomState state) = 0;

    /// Overloaded method which uses the index instead of the object pointer for faster access.
    virtual void setUserRoomState(qsizetype index, UserRoomState state) = 0;

    /// Whether the user is has joined the room.
    virtual bool isUserMemberOfRoom(const QString &userId) const = 0;

    /// Returns the ChatUser object pointer for the given id or nullptr.
    virtual ChatUser *chatUserById(const QString &userId) const = 0;

    /// List of users of this room, sorted by their display name (primarily) or id
    /// (secondarily). The list or its content must not be modified.
    virtual const QList<ChatUser *> &chatUsers() const = 0;

    Q_INVOKABLE virtual UserRoomState chatUserRoomState(ChatUser *user) const = 0;

    /// List of all users which are currently typing, ordered by their display name
    /// (primarily) or id (secondarily). The list or its content must not be modified.
    virtual const QList<ChatUser *> &typingUsers() const = 0;

    /// Remove all messages. Must invoke chatMessagesReset() afterwards.
    virtual void clear() = 0;

private:
    RoomSettings m_roomSettings;
    QDateTime m_latestMessageDateTime;
    bool m_isLoadingMessageHistory = false;
    bool m_isCompletelyLoaded = false;

Q_SIGNALS:
    void roomSettingsChanged();
    void nameChanged(QString name);
    void avatarPathChanged();
    void isFavoriteChanged();
    void notificationCountChanged(qsizetype count);
    void joinRuleChanged();
    void isDirectChatChanged();
    void isLoadingMessageHistoryChanged();
    void hasPresenceStateChanged();
    void presenceStateChanged();
    void permissionsChanged();
    void isInitiallyLoadedChanged();
    void isCompletelyLoadedChanged();
    void latestMessageDateTimeChanged();
    void ownUserJoinStateChanged();
    void otherUserChanged();

    /// Send when a chat message has been added. index is the one in the list returned by
    /// chatMessages(). Ownership remains in this room object.
    void chatMessageAdded(qsizetype index, ChatMessage *chatMessage);

    /// Send when a chat message has been added but not in the indexed list.
    void chatMessageOutOfSequenceReceived(ChatMessage *chatMessage);

    /// Send when a chat message has been removed. index is the one in the list returned by
    /// chatMessages() before the message has been removed. The ChatMessage object is deleted
    /// right after sending the message.
    void chatMessageRemoved(qsizetype index, ChatMessage *chatMessage);

    void chatMessageContentChanged(qsizetype index, ChatMessage *chatMessage);
    void chatMessageFlagsChanged(qsizetype index, ChatMessage *chatMessage,
                                 ChatMessage::Flags previousFlags);
    void chatMessageReactionsChanged(qsizetype index, ChatMessage *chatMessage);
    void chatMessageMentionedUsersChanged(qsizetype index, ChatMessage *chatMessage);

    /// Send when chat messages have been cleared (i.e. removed and deleted). All objects have been
    /// destroyed at this moment.
    void chatMessagesReset();

    /// Meta signal for both chatUserAdded and chatUserRemoved, i.e. send whenever one
    /// of the other two is emitted.
    void chatUsersChanged();
    void chatUserAdded(qsizetype index, ChatUser *user, IChatRoom::UserRoomState state);
    void chatUserRemoved(qsizetype index, ChatUser *user);
    void chatUserRoomStateChanged(qsizetype index, ChatUser *user, IChatRoom::UserRoomState state);

    /// Send when a user started or stopped typing. Use typingUsers() to retrieve the
    /// list of currently typing users.
    void typingUsersChanged();
};
