#pragma once

#include <QObject>
#include <QHash>

#include "ChatMessage.h"

class IChatRoom;

class ChatMessageContainer : public QObject
{
    Q_OBJECT

public:
    explicit ChatMessageContainer(IChatRoom *parent);
    virtual ~ChatMessageContainer();
    void clear();

    IChatRoom *chatRoom() const;

    qsizetype count() const { return m_messages.count(); }
    ChatMessage *at(qsizetype index) const;
    ChatMessage *messageById(const QString &messageId) const;
    qsizetype indexOf(const ChatMessage *message) const;
    QList<ChatMessage *> chatMessages() const { return m_messages; }
    bool contains(const QString &messageId) const { return m_messageLookup.contains(messageId); }
    bool contains(const ChatMessage *message) const;

    qsizetype unreadCount() const { return m_unreadCount; }
    void setUnreadCount(qsizetype count);

    /// Add a message to the container. Does not send the message. Takes ownership of the object.
    void addMessage(ChatMessage *message, bool isUnread, bool isIndependent);
    ChatMessage *removeMessage(const QString &messageId);
    ChatMessage *updateMessageEventId(const QString &oldEventId, const QString &newEventId);

private:
    QList<ChatMessage *> m_messages;
    QHash<QString, ChatMessage *> m_messageLookup;
    qsizetype m_unreadCount = 0;

Q_SIGNALS:
    void unreadCountChanged();

    void chatMessageAdded(qsizetype index, ChatMessage *chatMessage);
    void chatMessageOutOfSequenceReceived(ChatMessage *chatMessage);
    void chatMessageRemoved(qsizetype index, ChatMessage *chatMessage);
    void chatMessageContentChanged(qsizetype index, ChatMessage *chatMessage);
};
