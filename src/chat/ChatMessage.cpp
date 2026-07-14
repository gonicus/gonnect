#include "ChatMessage.h"
#include "ChatMessageContentUserStateChange.h"
#include "ChatMessageReaction.h"
#include "IChatRoom.h"

ChatMessage::ChatMessage(const QString &eventId, const QString &fromId, const QString &nickName,
                         QObject *content, const QDateTime &timestamp, IChatRoom *chatRoom,
                         Flags flags = Flag::Unknown)
    : QObject{ chatRoom },
      m_eventId{ eventId },
      m_fromId{ fromId },
      m_nickName{ nickName },
      m_timestamp{ timestamp },
      m_flags{ flags },
      m_content{ content },
      m_chatRoom{ chatRoom }
{
    Q_CHECK_PTR(chatRoom);

    if (content) {
        content->setParent(this);

        if (auto *textContent = qobject_cast<ChatMessageContentText *>(content)) {
            textContent->processText();
        }
    }
}

ChatMessage::~ChatMessage()
{
    qDeleteAll(m_reactions);

    if (m_content) {
        m_content->deleteLater();
        m_content = nullptr;
    }
}

void ChatMessage::setEventId(const QString &eventId)
{
    if (m_eventId != eventId) {
        m_eventId = eventId;
        Q_EMIT eventIdChanged();
    }
}

void ChatMessage::setContent(QObject *content)
{
    if (m_content != content) {

        if (m_content) {
            m_content->deleteLater();
            m_content = nullptr;
        }

        m_content = content;

        if (content) {
            content->setParent(this);

            if (auto *textContent = qobject_cast<ChatMessageContentText *>(content)) {
                textContent->processText();
            }
        }

        Q_EMIT contentChanged();
    }
}

bool ChatMessage::isStateUpdate() const
{
    return qobject_cast<ChatMessageContentUserStateChange *>(m_content);
}

ChatMessageContentUserStateChange::State ChatMessage::state() const
{
    if (const auto content = qobject_cast<ChatMessageContentUserStateChange *>(m_content)) {
        return content->state();
    }
    return ChatMessageContentUserStateChange::State::Unknown;
}

QString ChatMessage::affectedUserId() const
{
    if (const auto content = qobject_cast<ChatMessageContentUserStateChange *>(m_content)) {
        return content->affectedUserId();
    }
    return "";
}

void ChatMessage::updateReaction(const QString &reaction, const QSet<ChatUser *> users)
{
    auto r = m_reactions.value(reaction, nullptr);
    if (r) {
        if (users.isEmpty()) {
            m_reactions.remove(reaction);
            r->deleteLater();

        } else {
            const auto obsoleteUsers = r->users() - users;
            for (auto user : obsoleteUsers) {
                r->removeUser(user);
            }

            const auto newUsers = users - r->users();
            for (auto user : newUsers) {
                r->addUser(user);
            }
        }

    } else {
        r = new ChatMessageReaction(reaction);
        for (auto user : users) {
            r->addUser(user);
        }

        m_reactions.insert(reaction, r);
    }
}

void ChatMessage::addReaction(const QString &reaction, ChatUser *user)
{
    if (!user) {
        return;
    }

    if (auto r = m_reactions.value(reaction, nullptr)) {
        r->addUser(user);
    } else {
        r = new ChatMessageReaction(reaction);
        r->addUser(user);
        m_reactions.insert(reaction, r);
    }
}

void ChatMessage::removeReaction(const QString &reaction, ChatUser *user)
{
    if (!user) {
        return;
    }

    if (auto r = m_reactions.value(reaction, nullptr)) {
        r->removeUser(user);

        if (r->users().isEmpty()) {
            m_reactions.remove(reaction);
            r->deleteLater();
        }
    }
}

QList<const ChatMessageReaction *> ChatMessage::reactions() const
{
    QList<const ChatMessageReaction *> result;
    result.reserve(m_reactions.size());

    for (const auto *reaction : std::as_const(m_reactions)) {
        result.append(reaction);
    }

    std::sort(result.begin(), result.end(),
              [](const ChatMessageReaction *a, const ChatMessageReaction *b) {
                  if (a->count() == b->count()) {
                      return a->reaction() < b->reaction();
                  }
                  return a->count() > b->count();
              });

    return result;
}

QSet<ChatUser *> ChatMessage::mentionedUsers() const
{
    return m_mentionedPartpicipants;
}

void ChatMessage::addMentionendUser(ChatUser *user)
{
    Q_CHECK_PTR(user);
    m_mentionedPartpicipants.insert(user);
}

void ChatMessage::addMentionendUsers(const QSet<ChatUser *> users)
{
    m_mentionedPartpicipants.unite(users);
}

void ChatMessage::removeMentionendUser(ChatUser *user)
{
    Q_CHECK_PTR(user);
    m_mentionedPartpicipants.remove(user);
}

void ChatMessage::removeMentionendUsers(const QSet<ChatUser *> users)
{
    m_mentionedPartpicipants.subtract(users);
}
