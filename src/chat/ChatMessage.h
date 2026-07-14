#pragma once

#include <QObject>
#include <QDateTime>
#include <qqmlintegration.h>
#include "ChatMessageContentText.h"
#include "ChatMessageContentUserStateChange.h"

class ChatMessageReaction;
class ChatUser;
class IChatRoom;

class ChatMessage : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

    Q_PROPERTY(QString eventId READ eventId NOTIFY eventIdChanged FINAL)
    Q_PROPERTY(QString nickName READ nickName CONSTANT FINAL)
    Q_PROPERTY(bool isStateUpdate READ isStateUpdate CONSTANT FINAL)
    Q_PROPERTY(ChatMessageContentUserStateChange::State state READ state CONSTANT FINAL)
    Q_PROPERTY(QString affectedUserId READ affectedUserId CONSTANT FINAL)
    Q_PROPERTY(QObject *content READ content NOTIFY contentChanged FINAL)

public:
    enum class Flag {
        Unknown = 0,
        PrivateMessage = 1 << 0,
        SystemMessage = 1 << 1,
        OwnMessage = 1 << 2,
        Markdown = 1 << 3,
        Pinned = 1 << 4,
        Encrypted = 1 << 5,
        Pending = 1 << 6,
        Failed = 1 << 7,
    };
    Q_ENUM(Flag)
    Q_DECLARE_FLAGS(Flags, Flag)
    Q_FLAG(Flags)

    explicit ChatMessage(const QString &eventId, const QString &fromId, const QString &nickName,
                         QObject *content, const QDateTime &timestamp, IChatRoom *chatRoom,
                         Flags flags);

    virtual ~ChatMessage();

    QString eventId() const { return m_eventId; }
    void setEventId(const QString &eventId);
    QString fromId() const { return m_fromId; }
    QString nickName() const { return m_nickName; }
    QDateTime timestamp() const { return m_timestamp; }
    Flags flags() const { return m_flags; }
    QObject *content() const { return m_content; }
    IChatRoom *chatRoom() const { return m_chatRoom; }
    void setContent(QObject *content);

    bool isStateUpdate() const;

    ChatMessageContentUserStateChange::State state() const;
    QString affectedUserId() const;

    QString relatedMessageId() const { return m_relatedMessageId; }
    void setRelatedMessageId(const QString &id) { m_relatedMessageId = id; }

    void setFlags(Flags flags) { m_flags = flags; }

    /// Update (add, remove or change count) of a reaction (e.g. an emoji). An empty set will remove
    /// the reaction.
    void updateReaction(const QString &reaction, const QSet<ChatUser *> users);

    /// Add the reaction for the given user. If it already has the reaction, it does nothing.
    void addReaction(const QString &reaction, ChatUser *user);

    /// Remove the reaction for the given user. If it the reaction is non-existent, it does
    /// nothing.
    void removeReaction(const QString &reaction, ChatUser *user);

    /// Current reactions to this message, ordered by their individual count, descending.
    QList<const ChatMessageReaction *> reactions() const;

    /// List of users that have been mentioned in the message.
    QSet<ChatUser *> mentionedUsers() const;

    void addMentionendUser(ChatUser *user);
    void addMentionendUsers(const QSet<ChatUser *> users);
    void removeMentionendUser(ChatUser *user);
    void removeMentionendUsers(const QSet<ChatUser *> users);

private:
    QString m_eventId;
    QString m_fromId;
    QString m_nickName;
    QDateTime m_timestamp;
    Flags m_flags;
    QObject *m_content = nullptr;
    IChatRoom *m_chatRoom = nullptr;
    QString m_relatedMessageId;

    /// Hash map of reactions (e.g. emoji) to reaction objects for this message.
    QHash<QString, ChatMessageReaction *> m_reactions;

    QSet<ChatUser *> m_mentionedPartpicipants;

Q_SIGNALS:
    void contentChanged();
    void eventIdChanged();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ChatMessage::Flags)
