#pragma once

#include <QObject>
#include <QDateTime>

struct ChatMessageReaction
{
    QString emoji;
    qsizetype count = 0;
    bool hasOwnReaction = false;
};

class ChatMessage
{
    Q_GADGET
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

public:
    enum class Flag {
        PrivateMessage = 1 << 0,
        SystemMessage = 1 << 1,
        OwnMessage = 1 << 2,
        Markdown = 1 << 3
    };
    Q_ENUM(Flag)
    Q_DECLARE_FLAGS(Flags, Flag)
    Q_FLAG(Flags)

    explicit ChatMessage(const QString &eventId, const QString &fromId, const QString &nickName,
                         const QString &message, const QDateTime &timestamp,
                         Flags flags = static_cast<Flag>(0));

    QString eventId() const { return m_eventId; }
    QString fromId() const { return m_fromId; }
    QString nickName() const { return m_nickName; }
    QString message() const { return m_message; }
    QDateTime timestamp() const { return m_timestamp; }
    Flags flags() const { return m_flags; }
    void setFlags(Flags flags) { m_flags = flags; }

    void setReactionCount(const QString &reaction, qsizetype count, bool hasOwnReaction = false);
    const QList<ChatMessageReaction> &sortedReactions() const { return m_sortedReactions; }

private:
    QString m_eventId;
    QString m_fromId;
    QString m_nickName;
    QString m_message;
    QDateTime m_timestamp;
    Flags m_flags;
    QList<ChatMessageReaction> m_sortedReactions;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ChatMessage::Flags)
