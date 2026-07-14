#include "ChatModel.h"
#include "ChatMessage.h"
#include "ChatMessageReaction.h"
#include "IChatProvider.h"
#include "ChatMessageContentUserStateChange.h"
#include "IpcChatRoom.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcChatModel, "gonnect.app.chat.ChatModel")

ChatModel::ChatModel(QObject *parent) : QAbstractListModel{ parent }
{
    connect(this, &ChatModel::chatRoomChanged, this, &ChatModel::onChatRoomChanged);
}

QHash<int, QByteArray> ChatModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::EventId), "eventId" },
        { static_cast<int>(Roles::RoomId), "roomId" },
        { static_cast<int>(Roles::FromId), "fromId" },
        { static_cast<int>(Roles::NickName), "nickName" },
        { static_cast<int>(Roles::AvatarPath), "avatarPath" },
        { static_cast<int>(Roles::UserState), "userState" },
        { static_cast<int>(Roles::AffectedUserId), "affectedUserId" },
        { static_cast<int>(Roles::Timestamp), "timestamp" },
        { static_cast<int>(Roles::Reactions), "reactions" },
        { static_cast<int>(Roles::Content), "content" },

        { static_cast<int>(Roles::IsPrivateMessage), "isPrivateMessage" },
        { static_cast<int>(Roles::IsOwnMessage), "isOwnMessage" },
        { static_cast<int>(Roles::IsSystemMessage), "isSystemMessage" },
        { static_cast<int>(Roles::IsEncrypted), "isEncrypted" },
        { static_cast<int>(Roles::IsPinned), "isPinned" },
        { static_cast<int>(Roles::IsPending), "isPending" },
        { static_cast<int>(Roles::IsFailed), "isFailed" },
        { static_cast<int>(Roles::IsLastOwnMessage), "isLastOwnMessage" },
        { static_cast<int>(Roles::IsSameUserAsPrevious), "isSameUserAsPrevious" },
        { static_cast<int>(Roles::IsSameMinuteAsPrevious), "isSameMinuteAsPrevious" },
        { static_cast<int>(Roles::IsSameDayAsPrevious), "isSameDayAsPrevious" },
        { static_cast<int>(Roles::IsStateUpdate), "isStateUpdate" },

        { static_cast<int>(Roles::HasRelatedMessage), "hasRelatedMessage" },
        { static_cast<int>(Roles::RelatedMessageNickName), "relatedMessageNickName" },
        { static_cast<int>(Roles::RelatedMessageIsStateUpdate), "relatedMessageIsStateUpdate" },
        { static_cast<int>(Roles::RelatedMessageUserState), "relatedMessageUserState" },
        { static_cast<int>(Roles::RelatedMessageAffectedUserId), "relatedMessageAffectedUserId" },
        { static_cast<int>(Roles::RelatedMessageContent), "relatedMessageContent" },

        { static_cast<int>(Roles::MentionedUserNames), "mentionedUserNames" },
    };
}

int ChatModel::toNormalRole(const int role)
{
    return static_cast<int>(toNormalRole(static_cast<Roles>(role)));
}

ChatModel::Roles ChatModel::toNormalRole(const Roles role)
{
    static const QHash<Roles, Roles> convMap = {
        { Roles::RelatedMessageNickName, Roles::NickName },
        { Roles::RelatedMessageIsStateUpdate, Roles::IsStateUpdate },
        { Roles::RelatedMessageUserState, Roles::UserState },
        { Roles::RelatedMessageAffectedUserId, Roles::AffectedUserId },
        { Roles::RelatedMessageContent, Roles::Content },
    };

    return convMap.value(role, role);
}

int ChatModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_chatRoom ? m_chatRoom->chatMessages().size() : 0;
}

QVariant ChatModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !m_chatRoom) {
        return QVariant();
    }

    // Find row of related message
    const auto normalizedRole = toNormalRole(role);
    if (normalizedRole != role) {
        const auto &messages = m_chatRoom->chatMessages();
        const auto item = messages.at(index.row());

        if (item->relatedMessageId().isEmpty()) {
            return QVariant();
        }

        if (auto *room = qobject_cast<IpcChatRoom *>(m_chatRoom)) {
            room->ensureMessageLoaded(item->relatedMessageId());
        }

        const auto relatedMessage = m_chatRoom->chatMessageById(item->relatedMessageId());
        if (!relatedMessage) {
            return QVariant();
        }
        const auto relatedIndex = messages.indexOf(relatedMessage);
        if (relatedIndex >= 0) {
            return rawData(messages.at(relatedIndex), normalizedRole);
        }
        return rawData(relatedMessage, normalizedRole);
    }

    const auto item = m_chatRoom->chatMessages().at(index.row());

    if (role == static_cast<int>(Roles::IsSameUserAsPrevious)) {
        if (index.row() > 0) {
            if (const auto prev = m_chatRoom->chatMessages().at(index.row() - 1)) {
                return prev->fromId() == item->fromId();
            }
        }
        return false;
    }

    if (role == static_cast<int>(Roles::IsSameMinuteAsPrevious)) {
        if (index.row() > 0) {
            if (const auto prev = m_chatRoom->chatMessages().at(index.row() - 1)) {
                const auto prevDateTime = prev->timestamp();
                const auto itemDateTime = item->timestamp();
                return prevDateTime.date() == itemDateTime.date()
                        && prevDateTime.time().hour() == itemDateTime.time().hour()
                        && prevDateTime.time().minute() == itemDateTime.time().minute();
            }
        }
        return false;
    }

    if (role == static_cast<int>(Roles::IsSameDayAsPrevious)) {
        if (index.row() > 0) {
            if (const auto prev = m_chatRoom->chatMessages().at(index.row() - 1)) {
                return prev->timestamp().date() == item->timestamp().date();
            }
        }
        return false;
    }

    return rawData(item, role);
}

QVariant ChatModel::rawData(const ChatMessage *item, int role) const
{
    if (!item) {
        return QVariant();
    }

    switch (role) {
    case static_cast<int>(Roles::EventId):
        return item->eventId();

    case static_cast<int>(Roles::FromId):
        return item->fromId();

    case static_cast<int>(Roles::NickName):
        return item->nickName();

    case static_cast<int>(Roles::RoomId):
        return m_chatRoom->id();

    case static_cast<int>(Roles::AvatarPath): {
        if (const auto user = m_chatRoom->chatUserById(item->fromId())) {
            return user->avatarPath();
        }
        return QString();
    }

    case static_cast<int>(Roles::Timestamp):
        return item->timestamp();

    case static_cast<int>(Roles::Reactions): {
        QVariantList l;
        const auto reactions = item->reactions();
        auto chatProvider = qobject_cast<IChatProvider *>(m_chatRoom->parent());
        QString ownUserId;
        if (chatProvider) {
            ownUserId = chatProvider->ownUserId();
        }
        for (const auto *reaction : reactions) {
            QVariantMap m;
            m.insert("reaction", reaction->reaction());
            m.insert("count", reaction->count());
            m.insert("isOwnReaction", ownUserId.isEmpty() ? false : reaction->isUser(ownUserId));
            l.append(m);
        }
        return l;
    }

    case static_cast<int>(Roles::Content):
        return QVariant::fromValue(item->content());

    case static_cast<int>(Roles::IsStateUpdate):
        return qobject_cast<ChatMessageContentUserStateChange *>(item->content()) != nullptr;

    case static_cast<int>(Roles::UserState): {
        if (const auto stateContent =
                    qobject_cast<ChatMessageContentUserStateChange *>(item->content())) {
            return QVariant::fromValue(stateContent->state());
        }
        return QVariant::fromValue(ChatMessageContentUserStateChange::State::Unknown);
    }

    case static_cast<int>(Roles::AffectedUserId): {
        if (const auto stateContent =
                    qobject_cast<ChatMessageContentUserStateChange *>(item->content())) {
            return stateContent->affectedUserId();
        }
        return QString();
    }

    case static_cast<int>(Roles::IsPrivateMessage):
        return static_cast<bool>(item->flags() & ChatMessage::Flag::PrivateMessage);

    case static_cast<int>(Roles::IsOwnMessage):
        return static_cast<bool>(item->flags() & ChatMessage::Flag::OwnMessage);

    case static_cast<int>(Roles::IsSystemMessage):
        return static_cast<bool>(item->flags() & ChatMessage::Flag::SystemMessage);

    case static_cast<int>(Roles::IsEncrypted):
        return static_cast<bool>(item->flags() & ChatMessage::Flag::Encrypted);

    case static_cast<int>(Roles::IsPinned):
        return static_cast<bool>(item->flags() & ChatMessage::Flag::Pinned);

    case static_cast<int>(Roles::IsPending):
        return static_cast<bool>(item->flags() & ChatMessage::Flag::Pending);

    case static_cast<int>(Roles::IsFailed):
        return static_cast<bool>(item->flags() & ChatMessage::Flag::Failed);

    case static_cast<int>(Roles::IsLastOwnMessage):
        return item == m_lastOwnMessage;

    case static_cast<int>(Roles::HasRelatedMessage):
        return !item->relatedMessageId().isEmpty();

    case static_cast<int>(Roles::MentionedUserNames): {
        QStringList names;
        const auto users = item->mentionedUsers();
        names.reserve(users.size());
        for (const auto user : users) {
            names.append(user->computedName());
        }
        return names;
    }
    }

    return QVariant();
}

void ChatModel::onChatRoomChanged()
{
    beginResetModel();

    if (m_chatRoomContext) {
        m_chatRoomContext->deleteLater();
        m_chatRoomContext = nullptr;
    }

    if (m_chatRoom) {
        m_chatRoomContext = new QObject(this);
        connect(m_chatRoom, &IChatRoom::chatMessageAdded, m_chatRoomContext,
                [this](qsizetype index, ChatMessage *msgObj) {
                    beginInsertRows(QModelIndex(), index, index);
                    endInsertRows();
                    updateRelatedMessages(msgObj->eventId(), relatedContentRoles(*msgObj));
                    updateRealMessagesCount();

                    // Update next item for "previous" roles
                    if (index < rowCount(QModelIndex()) - 1) {
                        const auto nextIndex = createIndex(index + 1, 0);
                        Q_EMIT dataChanged(nextIndex, nextIndex, nextItemContentRoles());
                    }

                    updateLastOwnMessage();
                });
        connect(m_chatRoom, &IChatRoom::chatMessageRemoved, m_chatRoomContext,
                [this](qsizetype index, ChatMessage *msgObj) {
                    beginRemoveRows(QModelIndex(), index, index);
                    endRemoveRows();
                    updateRelatedMessages(msgObj->eventId(), relatedContentRoles(*msgObj));
                    updateRealMessagesCount();

                    // Update next item for "previous" roles
                    // index is now the next after removing the row
                    if (index < rowCount(QModelIndex())) {
                        const auto nextIndex = createIndex(index, 0);
                        Q_EMIT dataChanged(nextIndex, nextIndex, nextItemContentRoles());
                    }

                    updateLastOwnMessage();
                });
        connect(m_chatRoom, &IChatRoom::chatMessageOutOfSequenceReceived, m_chatRoomContext,
                [this](ChatMessage *msgObj) {
                    if (!msgObj) {
                        return;
                    }
                    updateRelatedMessages(msgObj->eventId(), relatedContentRoles(*msgObj));
                });
        connect(m_chatRoom, &IChatRoom::chatMessagesReset, m_chatRoomContext, [this]() {
            beginResetModel();
            endResetModel();
            updateLastOwnMessage();
            updateRealMessagesCount();
        });
        connect(m_chatRoom, &IChatRoom::chatMessageContentChanged, m_chatRoomContext,
                [this](qsizetype idx, ChatMessage *msgObj) {
                    const auto modelIndex = createIndex(idx, 0);
                    Q_EMIT dataChanged(modelIndex, modelIndex,
                                       { static_cast<int>(Roles::Content) });

                    updateRelatedMessages(msgObj->eventId(), relatedContentRoles(*msgObj));
                });
        connect(m_chatRoom, &IChatRoom::chatMessageMentionedUsersChanged, m_chatRoomContext,
                [this](qsizetype idx, ChatMessage *) {
                    const auto modelIndex = createIndex(idx, 0);
                    Q_EMIT dataChanged(modelIndex, modelIndex,
                                       { static_cast<int>(Roles::MentionedUserNames) });
                });
        connect(m_chatRoom, &IChatRoom::chatMessageFlagsChanged, m_chatRoomContext,
                [this](qsizetype idx, ChatMessage *chatMessage, ChatMessage::Flags previousFlags) {
                    if (!chatMessage) {
                        qCCritical(lcChatModel)
                                << "Ignoring IChatRoom::chatMessageFlagsChanged signal with "
                                   "nullptr for parameter ChatMessage* chatMessage";
                        return;
                    }

                    QList<int> affectedRoles;
                    const auto currentFlags = chatMessage->flags();
                    const auto changedFlags = previousFlags ^ currentFlags;

                    if (changedFlags & ChatMessage::Flag::Encrypted) {
                        affectedRoles.append(static_cast<int>(Roles::IsEncrypted));
                        affectedRoles.append(static_cast<int>(Roles::Content));
                    }
                    if (changedFlags & ChatMessage::Flag::Pinned) {
                        affectedRoles.append(static_cast<int>(Roles::IsPinned));
                    }
                    if (changedFlags & ChatMessage::Flag::Pending) {
                        affectedRoles.append(static_cast<int>(Roles::IsPending));
                    }
                    if (changedFlags & ChatMessage::Flag::Failed) {
                        affectedRoles.append(static_cast<int>(Roles::IsFailed));
                    }

                    const auto modelIndex = createIndex(idx, 0);
                    Q_EMIT dataChanged(modelIndex, modelIndex, affectedRoles);

                    if (changedFlags
                        & (ChatMessage::Flag::OwnMessage | ChatMessage::Flag::Pending
                           | ChatMessage::Flag::Failed)
                        && m_lastOwnMessage != nullptr
                        && (chatMessage == m_lastOwnMessage
                            || changedFlags & ChatMessage::Flag::OwnMessage)) {
                        updateLastOwnMessage();
                    }
                });
        connect(m_chatRoom, &IChatRoom::chatMessageReactionsChanged, m_chatRoomContext,
                [this](qsizetype idx, ChatMessage *) {
                    const auto modelIndex = createIndex(idx, 0);
                    Q_EMIT dataChanged(modelIndex, modelIndex,
                                       { static_cast<int>(Roles::Reactions) });
                });
    }

    endResetModel();
    updateLastOwnMessage();
    updateRealMessagesCount();
}

void ChatModel::updateRealMessagesCount()
{
    uint count = 0;

    if (m_chatRoom) {
        const auto &messages = m_chatRoom->chatMessages();
        for (const auto &message : messages) {
            if (!(message->flags() & ChatMessage::Flag::SystemMessage)) {
                ++count;
            }
        }
    }

    if (m_realMessagesCount != count) {
        m_realMessagesCount = count;
        Q_EMIT realMessagesCountChanged();
    }
}

ChatMessage *ChatModel::relatedMessage(ChatMessage *originalMessage) const
{
    if (originalMessage && !originalMessage->relatedMessageId().isEmpty()) {
        return m_chatRoom->chatMessageById(originalMessage->relatedMessageId());
    }

    return nullptr;
}

void ChatModel::updateRelatedMessages(const QString &originalMessageId, const QList<int> &roles)
{
    if (originalMessageId.isEmpty() || roles.isEmpty()) {
        return;
    }

    const auto messages = m_chatRoom->chatMessages();
    const qsizetype l = messages.length();

    for (qsizetype i = 0; i < l; ++i) {
        const auto chatMsg = messages.at(i);
        if (chatMsg->relatedMessageId() == originalMessageId) {
            const auto modelIndex = createIndex(i, 0);
            Q_EMIT dataChanged(modelIndex, modelIndex, roles);
        }
    }
}

QList<int> ChatModel::nextItemContentRoles()
{
    static const QList<int> roles = {
        static_cast<int>(Roles::IsSameDayAsPrevious),
        static_cast<int>(Roles::IsSameMinuteAsPrevious),
        static_cast<int>(Roles::IsSameUserAsPrevious),
    };
    return roles;
}

QList<int> ChatModel::relatedContentRoles(const ChatMessage &messageObject) const
{
    QList<int> roles = { static_cast<int>(Roles::RelatedMessageNickName),
                         static_cast<int>(Roles::RelatedMessageContent) };
    const auto content = messageObject.content();

    if (qobject_cast<ChatMessageContentUserStateChange *>(content)) {
        roles.append(static_cast<int>(Roles::RelatedMessageUserState));
        roles.append(static_cast<int>(Roles::RelatedMessageAffectedUserId));
    }

    return roles;
}

void ChatModel::updateLastOwnMessage()
{
    m_lastOwnMessage = nullptr;
    if (!m_chatRoom) {
        return;
    }
    const auto &messages = m_chatRoom->chatMessages();
    for (const auto msg : messages) {
        if (msg->flags() & ChatMessage::Flag::OwnMessage
                && !(msg->flags() & ChatMessage::Flag::Pending)
                && !(msg->flags() & ChatMessage::Flag::Failed)) {
            m_lastOwnMessage = msg;
        }
    }
    const auto lastTop = createIndex(0, 0);
    const auto lastBottom = createIndex(rowCount(QModelIndex()) - 1, 0);
    Q_EMIT dataChanged(lastTop, lastBottom,
                       { static_cast<int>(Roles::IsLastOwnMessage) });
}
