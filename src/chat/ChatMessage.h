#pragma once

#include <QObject>
#include <QDateTime>

class ChatMessage
{
    Q_GADGET
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

public:
    enum class Flag {
        Unknown = 0,
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

private:
    QString m_eventId;
    QString m_fromId;
    QString m_nickName;
    QString m_message;
    QDateTime m_timestamp;
    Flags m_flags;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ChatMessage::Flags)
