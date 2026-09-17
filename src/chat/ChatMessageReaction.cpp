#include "ChatMessageReaction.h"
#include "ChatUser.h"

ChatMessageReaction::ChatMessageReaction(const QString &reaction, QObject *parent)
    : QObject{ parent }, m_reaction{ reaction }
{
}

void ChatMessageReaction::addUser(ChatUser *user)
{
    Q_CHECK_PTR(user);

    if (!m_users.contains(user)) {

        connect(user, &ChatUser::destroyed, this, [this](QObject *obj) {
            if (auto user = qobject_cast<ChatUser *>(obj)) {
                removeUser(user);
            }
        });

        m_users.insert(user);
        Q_EMIT countChanged(count());
        Q_EMIT chatUserAdded(user);
    }
}

void ChatMessageReaction::removeUser(ChatUser *user)
{
    Q_CHECK_PTR(user);

    if (m_users.contains(user)) {
        m_users.remove(user);
        user->disconnect(this);
        Q_EMIT countChanged(count());
        Q_EMIT chatUserRemoved(user);
    }
}

qsizetype ChatMessageReaction::count() const
{
    return std::as_const(m_users).size();
}

bool ChatMessageReaction::isUser(const QString &userId) const
{
    for (const auto *user : std::as_const(m_users)) {
        if (user->id() == userId) {
            return true;
        }
    }
    return false;
}
