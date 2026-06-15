#pragma once

#include "IChatRoom.h"
#include <QHash>

class ChatUser;
class IpcDispatcher;

class IpcChatRoom : public IChatRoom
{
    Q_OBJECT

public:
    explicit IpcChatRoom(const QString &id, const QString &name, QObject *parent = nullptr);
    virtual ~IpcChatRoom();

    void setName(const QString &name);
    void setAvatarPath(const QString &path);
    void setJoinRule(IChatRoom::JoinRule joinRule);
    void setIsDirect(bool value);
    void setUnreadCount(qsizetype count);
    void setPermissions(IChatRoom::Permissions permissions);
    void setIsFavorite(bool value);

    virtual QString id() override { return m_id; };
    virtual QString name() override { return m_name; };
    virtual QString avatarPath() override;
    virtual bool isFavorite() override { return m_isFavorite; }
    virtual IChatRoom::JoinRule joinRule() override { return m_joinRule; }
    virtual qsizetype notificationCount() override { return m_unreadCount; }
    virtual IChatRoom::Permissions permissions() override { return m_permissions; }

    virtual bool isInitiallyLoaded() const override { return m_isInitiallyLoaded; }
    virtual void loadMessages() override;

    virtual void resetUnreadCount() override;
    virtual QList<ChatMessage *> chatMessages() const override { return m_messages; }
    virtual ChatMessage *chatMessageById(const QString &id) const override;
    virtual ChatMessage *latestOwnTextMessage() const override;
    virtual void sendMessage(const QString &message, const QString &relatedMessageId = "") override;
    virtual void sendImage(const QString &filePath) override;
    virtual void sendFile(const QString &filePath) override;

    /// Add an already existing message to the room; does not send a new message. Takes ownership of
    /// the object.
    void addExistingMessage(ChatMessage *message, bool isUnread);

    bool hasMessage(const QString &messageId) const { return m_messageLookup.contains(messageId); }
    bool hasMessage(const ChatMessage *message) const { return m_messages.contains(message); }
    qsizetype indexOfMessage(const ChatMessage *message) const;

    void removeMessage(const QString &messageId);

    virtual bool isDirectChat() override { return m_isDirectChat; }
    virtual bool hasPresenceState() override;
    virtual ChatUser::PresenceState presenceState() const override;
    virtual IChatRoom::UserRoomState ownUserJoinState() const override;
    virtual ChatUser *otherUser() const override;
    virtual qsizetype chatUserCount() const override { return m_chatUsers.length(); }
    virtual void addUser(ChatUser *user, UserRoomState state) override;
    virtual void removeUser(ChatUser *user) override;
    virtual void setUserRoomState(ChatUser *user, UserRoomState state) override;
    virtual void setUserRoomState(qsizetype index, UserRoomState state) override;
    virtual ChatUser *chatUserById(const QString &userId) const override;
    virtual bool isUserMemberOfRoom(const QString &userId) const override;
    virtual const QList<ChatUser *> &chatUsers() const override;
    virtual UserRoomState chatUserRoomState(ChatUser *user) const override;
    virtual const QList<ChatUser *> &typingUsers() const override;
    virtual void clear() override;

    void setTypingUsers(const QList<ChatUser *> &users);

private Q_SLOTS:
    void updateIsDirectChat();
    void updateOtherUser();
    void updateOwnUserJoinState(qsizetype index, ChatUser *user, IChatRoom::UserRoomState state);

private:
    IpcDispatcher *ipcDispatcher() const;

    QString m_id;
    QString m_name;
    QString m_avatarPath;
    qsizetype m_unreadCount = 0;
    IChatRoom::JoinRule m_joinRule = IChatRoom::JoinRule::Unknown;
    IChatRoom::UserRoomState m_ownUserJoinState = IChatRoom::UserRoomState::Unjoined;
    IChatRoom::Permissions m_permissions;
    bool m_isFavorite = false;
    bool m_isDirectChat = true;
    bool m_isInitiallyLoaded = false;

    QList<ChatUser *> m_chatUsers;
    QHash<ChatUser *, UserRoomState> m_userRoomStates;
    QHash<QString, ChatUser *> m_chatUserLookup;
    QList<ChatUser *> m_typingUsers;
    ChatUser *m_otherUser = nullptr;
    QObject *m_otherUserContext = nullptr;

    QHash<QString, ChatMessage *> m_messageLookup;
    QList<ChatMessage *> m_messages;
};
