#pragma once

#include "IChatRoom.h"

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

    QString id() override { return m_id; }
    QString name() override { return m_name; }
    qsizetype notificationCount() override;
    void resetUnreadCount() override;
    QList<ChatMessage *> chatMessages() const override { return m_messages; }
    void sendMessage(const QString &message) override;
    void clear() override;

    /// Add a message object to be handled by this room. Takes ownership of that object.
    void addMessage(ChatMessage *chatMessageObj);

private:
    QString m_id;
    QString m_name;
    QList<ChatMessage *> m_messages;

Q_SIGNALS:
    void sendMessageRequested(QString message);
};
