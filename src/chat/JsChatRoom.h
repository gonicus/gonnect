#pragma once

#include "IChatRoom.h"

class JsChatConnector;

class JsChatRoom : public IChatRoom
{
    Q_OBJECT

public:
    explicit JsChatRoom(const QString &id, const QString &name, JsChatConnector *parent);

    virtual ~JsChatRoom();

    /// Takes ownership of the ChatMessage object
    void addMessage(ChatMessage *chatMessageObject);

    void setUnreadNotificationCount(qsizetype count);
    void setName(const QString &name);

    virtual QString id() override { return m_id; };
    virtual QString name() override { return m_name; };
    virtual qsizetype notificationCount() override { return m_unreadCount; };
    virtual void resetUnreadCount() override;
    virtual QList<ChatMessage *> chatMessages() const override { return m_messages; }
    virtual void sendMessage(const QString &message) override;

private:
    JsChatConnector *connectorParent() const;

    QString m_id;
    QString m_name;
    QList<ChatMessage *> m_messages;
    qsizetype m_unreadCount = 0;
};
