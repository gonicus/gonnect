#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>

#include "IChatRoom.h"

class ChatModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(IChatRoom *chatRoom MEMBER m_chatRoom NOTIFY chatRoomChanged FINAL)
    Q_PROPERTY(uint realMessagesCount READ realMessagesCount NOTIFY realMessagesCountChanged FINAL)

public:
    enum class Roles {
        EventId = Qt::UserRole + 1,
        RoomId,
        FromId,
        AvatarPath,
        Timestamp,
        NickName,
        UserState,
        AffectedUserId,
        Content,
        Reactions,

        IsPrivateMessage,
        IsOwnMessage,
        IsSystemMessage,
        IsEncrypted,
        IsPinned,
        IsSameUserAsPrevious,
        IsSameMinuteAsPrevious,
        IsSameDayAsPrevious,
        IsStateUpdate,

        HasRelatedMessage,
        RelatedMessageNickName,
        RelatedMessageIsStateUpdate,
        RelatedMessageUserState,
        RelatedMessageAffectedUserId,
        RelatedMessageContent,

        MentionedUserNames
    };
    Q_ENUM(Roles)

    explicit ChatModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    uint realMessagesCount() const { return m_realMessagesCount; }

private Q_SLOTS:
    void onChatRoomChanged();
    void updateRealMessagesCount();

private:
    static Roles toNormalRole(const Roles role);
    static int toNormalRole(const int role);

    QVariant rawData(int row, int role) const;
    ChatMessage *relatedMessage(ChatMessage *originalMessage) const;
    void updateRelatedMessages(const QString &originalMessageId, const QList<int> &roles);
    static QList<int> nextItemContentRoles();
    QList<int> relatedContentRoles(const ChatMessage &messageObject) const;

    IChatRoom *m_chatRoom = nullptr;
    QObject *m_chatRoomContext = nullptr;
    uint m_realMessagesCount = 0;

Q_SIGNALS:
    void chatRoomChanged();
    void realMessagesCountChanged();
};
