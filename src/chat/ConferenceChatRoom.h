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
    virtual ~ConferenceChatRoom();

    virtual QString id() override { return m_id; }
    virtual QString name() override { return m_name; }
    virtual qsizetype notificationCount() override;
    virtual void resetUnreadCount() override;
    virtual QList<ChatMessage *> chatMessages() const override { return m_messages; }
    virtual void sendMessage(const QString &message) override;
    virtual void toggleReaction(const QString &eventId, const QString &emoji) override;
    virtual void setReactionCount(const QString &eventId, const QString &emoji, qsizetype count,
                                  const QString &ownReactEventId) override;

    /// Add a message object to be handled by this room. Takes ownership of that object.
    void addMessage(ChatMessage *chatMessageObj);

private:
    QString m_id;
    QString m_name;
    QList<ChatMessage *> m_messages;

Q_SIGNALS:
    void sendMessageRequested(QString message);
};
