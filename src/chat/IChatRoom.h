#pragma once

#include <QObject>
#include <qqmlregistration.h>

class ChatMessage;

class IChatRoom : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by C++")

    Q_PROPERTY(QString id READ id CONSTANT FINAL)

public:
    explicit IChatRoom(QObject *parent = nullptr);
    virtual ~IChatRoom() { }

    virtual QString id() = 0;
    virtual QString name() = 0;
    virtual qsizetype notificationCount() = 0;
    Q_INVOKABLE virtual void resetUnreadCount() = 0;

    /// List of chat messages of this room, sorted by timestamp ascending
    virtual QList<ChatMessage *> chatMessages() const = 0;

    /// Send a message in this room
    Q_INVOKABLE virtual void sendMessage(const QString &message) = 0;

signals:
    void nameChanged(QString name);
    void notificationCountChanged(qsizetype count);

    /// Send when a chat message has been added. index is the one in the list returned by
    /// chatMessages(). Ownership remains in this room object.
    void chatMessageAdded(qsizetype index, ChatMessage *chatMessage);

    /// Send when a chat message has been removed. index is the one in the list returned by
    /// chatMessages() before the message has been removed. The ChatMessage object is deleted
    /// right after sending the message.
    void chatMessageRemoved(qsizetype index, ChatMessage *chatMessage);

    /// Send when chat messages have been cleared (i.e. removed and deleted). All objects have been
    /// destroyed at this moment.
    void chatMessagesReset();
};
