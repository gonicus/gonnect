#pragma once

#include <QObject>
#include <QSet>

class ChatUser;

class ChatMessageReaction : public QObject
{
    Q_OBJECT

public:
    explicit ChatMessageReaction(const QString &reaction, QObject *parent = nullptr);

    QString reaction() const { return m_reaction; }
    qsizetype count() const;
    const QSet<ChatUser *> &users() const { return m_users; }
    bool isUser(const QString &userId) const;

    void addUser(ChatUser *user);
    void removeUser(ChatUser *user);

private:
    QString m_reaction;
    QSet<ChatUser *> m_users;

Q_SIGNALS:
    void chatUserAdded(ChatUser *user);
    void chatUserRemoved(ChatUser *user);
    void countChanged(qsizetype count);
};
