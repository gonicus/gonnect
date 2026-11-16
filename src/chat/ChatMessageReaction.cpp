#include "ChatMessageReaction.h"
#include "ChatUser.h"

namespace {
constexpr QChar textPresentationSelector{ 0xFE0E };
constexpr QChar emojiPresentationSelector{ 0xFE0F };
} // namespace

ChatMessageReaction::ChatMessageReaction(const QString &reaction, QObject *parent)
    : QObject{ parent }, m_reaction{ reaction }
{
}

QString ChatMessageReaction::normalizedKey(const QString &reaction)
{
    QString key = reaction;
    key.remove(textPresentationSelector);
    key.remove(emojiPresentationSelector);
    return key;
}

void ChatMessageReaction::addUser(ChatUser *user, const QString &variant)
{
    Q_CHECK_PTR(user);

    const bool isEmojiVariant = variant.contains(emojiPresentationSelector);
    if (isEmojiVariant && !m_reaction.contains(emojiPresentationSelector)) {
        m_reaction = variant;
    }

    if (m_variantsByUser.contains(user)) {
        m_variantsByUser[user].insert(variant);
        return;
    }

    connect(user, &ChatUser::destroyed, this,
            [this, user](QObject *) { removeUserCompletely(user); });

    m_variantsByUser.insert(user, { variant });
    Q_EMIT countChanged(count());
    Q_EMIT chatUserAdded(user);
}

void ChatMessageReaction::removeUserCompletely(ChatUser *user)
{
    Q_CHECK_PTR(user);

    if (m_variantsByUser.remove(user)) {
        user->disconnect(this);
        Q_EMIT countChanged(count());
        Q_EMIT chatUserRemoved(user);
    }
}

void ChatMessageReaction::removeUser(ChatUser *user, const QString &variant)
{
    Q_CHECK_PTR(user);

    const auto it = m_variantsByUser.find(user);
    if (it == m_variantsByUser.end()) {
        return;
    }

    it->remove(variant);
    if (it->isEmpty()) {
        removeUserCompletely(user);
    }
}

qsizetype ChatMessageReaction::count() const
{
    return m_variantsByUser.size();
}

bool ChatMessageReaction::isUser(const QString &userId) const
{
    return !variantsOfUser(userId).isEmpty();
}

QSet<QString> ChatMessageReaction::variantsOfUser(const QString &userId) const
{
    for (const auto &[user, variants] : m_variantsByUser.asKeyValueRange()) {
        if (user->id() == userId) {
            return variants;
        }
    }
    return {};
}
