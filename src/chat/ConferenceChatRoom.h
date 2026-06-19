#pragma once

#include "IChatRoom.h"
#include "ChatUser.h"

#include <QDateTime>

class IConferenceConnector;

class ConferenceChatRoom : public IChatRoom
{
    Q_OBJECT
public:
    explicit ConferenceChatRoom(IConferenceConnector *parent = nullptr);

    explicit ConferenceChatRoom(const QString &roomId, const QString &name,
                                IConferenceConnector *parent = nullptr);
    ~ConferenceChatRoom();

    virtual QString id() override { return m_id; }
    virtual QString name() override { return m_name; }
    virtual QString avatarPath() override { return ""; }
    virtual bool isInitiallyLoaded() const override { return true; }
    virtual void loadMessages() override { }
    virtual qsizetype notificationCount() override;
    virtual void resetUnreadCount() override;
    virtual IChatRoom::JoinRule joinRule() override;
    virtual IChatRoom::Permissions permissions() override;
    virtual QList<ChatMessage *> chatMessages() const override { return m_messages; }
    virtual ChatMessage *chatMessageById(const QString &id) const override;
    virtual ChatMessage *latestOwnTextMessage() const override { return nullptr; }
    virtual void sendMessage(const QString &message, const QString &relatedMessageId = "") override;
    virtual void sendFile(const QString &filePath) override { Q_UNUSED(filePath) }
    virtual bool isDirectChat() override { return false; }
    virtual bool isFavorite() override { return false; }
    virtual bool hasPresenceState() override { return false; }
    virtual ChatUser::PresenceState presenceState() const override;
    virtual IChatRoom::UserRoomState ownUserJoinState() const override;
    virtual ChatUser *otherUser() const override { return nullptr; }
    virtual qsizetype chatUserCount() const override { return 0; }
    virtual void addUser(ChatUser *user, UserRoomState state) override;
    virtual void removeUser(ChatUser *user) override;
    virtual void setUserRoomState(ChatUser *user, UserRoomState state) override;
    virtual void setUserRoomState(qsizetype index, UserRoomState state) override;
    virtual ChatUser *chatUserById(const QString &userId) const override;
    virtual const QList<ChatUser *> &chatUsers() const override;
    virtual ConferenceChatRoom::UserRoomState chatUserRoomState(ChatUser *user) const override;
    virtual const QList<ChatUser *> &typingUsers() const override;
    virtual bool isUserMemberOfRoom(const QString &userId) const override;
    virtual void clear() override;

    /// Add a message object to be handled by this room. Takes ownership of that object.
    void addMessage(ChatMessage *chatMessageObj);

private:
    QString m_id;
    QString m_name;
    QList<ChatMessage *> m_messages;

Q_SIGNALS:
    void sendMessageRequested(QString message);
};
