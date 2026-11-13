#pragma once

#include <QHash>
#include <QObject>
#include <QSet>

class ChatUser;

class ChatMessageReaction : public QObject
{
    Q_OBJECT

public:
    explicit ChatMessageReaction(const QString &reaction, QObject *parent = nullptr);

    /// Normalized emojis (i.e. w/o variation selectors etc.)
    static QString normalizedKey(const QString &reaction);

    QString reaction() const { return m_reaction; }
    qsizetype count() const;
    QList<ChatUser *> users() const { return m_variantsByUser.keys(); }
    bool isUser(const QString &userId) const;
    QSet<QString> variantsOfUser(const QString &userId) const;

    void addUser(ChatUser *user, const QString &variant);
    void removeUser(ChatUser *user, const QString &variant);

private:
    void removeUserCompletely(ChatUser *user);

    QString m_reaction;
    QHash<ChatUser *, QSet<QString>> m_variantsByUser;

Q_SIGNALS:
    void chatUserAdded(ChatUser *user);
    void chatUserRemoved(ChatUser *user);
    void countChanged(qsizetype count);
};
