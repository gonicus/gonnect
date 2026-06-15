#include "ChatRoomModel.h"
#include "ChatUser.h"
#include "IChatProvider.h"
#include "IChatRoom.h"
#include "ChatMessage.h"

ChatRoomModel::ChatRoomModel(QObject *parent) : QAbstractListModel{ parent }
{
    connect(this, &ChatRoomModel::chatProviderChanged, this, &ChatRoomModel::onChatProviderChanged);
}

QHash<int, QByteArray> ChatRoomModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::RoomId), "roomId" },
        { static_cast<int>(Roles::Name), "name" },
        { static_cast<int>(Roles::AvatarPath), "avatarPath" },
        { static_cast<int>(Roles::UnreadCount), "unreadCount" },
        { static_cast<int>(Roles::IsFavorite), "isFavorite" },
        { static_cast<int>(Roles::HasPresenceState), "hasPresenceState" },
        { static_cast<int>(Roles::PresenceState), "presenceState" },
        { static_cast<int>(Roles::JoinRule), "joinRule" },
        { static_cast<int>(Roles::LatestMessageDate), "latestMessageDate" },
        { static_cast<int>(Roles::ChatProvider), "chatProvider" },
        { static_cast<int>(Roles::Permissions), "permissions" },
        { static_cast<int>(Roles::OwnJoinState), "ownJoinState" },
        { static_cast<int>(Roles::TypingUserNames), "typingUserNames" },
    };
}

QModelIndex ChatRoomModel::indexForRoomId(const QString &roomId) const
{
    if (m_chatProvider) {
        if (const auto chatRoom = m_chatProvider->chatRoomByRoomId(roomId)) {
            const auto row = m_chatProvider->indexOf(chatRoom);
            if (row >= 0) {
                return index(row);
            }
        }
    }

    return QModelIndex();
}

void ChatRoomModel::connectChatRoomSignals(IChatRoom *chatRoom)
{
    if (!chatRoom || m_chatRoomContextObjects.contains(chatRoom)) {
        return;
    }

    auto ctx = new QObject(this);

    connect(chatRoom, &QObject::destroyed, ctx, [this, chatRoom](QObject *) {
        const auto chatIndex = m_chatProvider->indexOf(chatRoom);
        if (chatIndex >= 0) {
            beginRemoveRows(QModelIndex(), chatIndex, chatIndex);
            if (auto *ctx = m_chatRoomContextObjects.take(chatRoom)) {
                ctx->deleteLater();
            }
            endRemoveRows();
        }
    });

    connect(chatRoom, &IChatRoom::nameChanged, ctx,
            [this, chatRoom]() { emitDataChanged(chatRoom, { static_cast<int>(Roles::Name) }); });
    connect(chatRoom, &IChatRoom::avatarPathChanged, ctx, [this, chatRoom]() {
        emitDataChanged(chatRoom, { static_cast<int>(Roles::AvatarPath) });
    });
    connect(chatRoom, &IChatRoom::isFavoriteChanged, ctx, [this, chatRoom]() {
        emitDataChanged(chatRoom, { static_cast<int>(Roles::IsFavorite) });
    });
    connect(chatRoom, &IChatRoom::joinRuleChanged, ctx, [this, chatRoom]() {
        emitDataChanged(chatRoom, { static_cast<int>(Roles::JoinRule) });
    });
    connect(chatRoom, &IChatRoom::ownUserJoinStateChanged, ctx, [this, chatRoom]() {
        emitDataChanged(chatRoom, { static_cast<int>(Roles::OwnJoinState) });
    });
    connect(chatRoom, &IChatRoom::notificationCountChanged, ctx, [this, chatRoom]() {
        emitDataChanged(chatRoom, { static_cast<int>(Roles::UnreadCount) });
    });
    connect(chatRoom, &IChatRoom::latestMessageDateTimeChanged, ctx, [this, chatRoom]() {
        emitDataChanged(chatRoom, { static_cast<int>(Roles::LatestMessageDate) });
    });
    connect(chatRoom, &IChatRoom::permissionsChanged, ctx, [this, chatRoom]() {
        emitDataChanged(chatRoom, { static_cast<int>(Roles::Permissions) });
    });
    connect(chatRoom, &IChatRoom::typingParticpantsChanged, ctx, [this, chatRoom]() {
        emitDataChanged(chatRoom, { static_cast<int>(Roles::TypingUserNames) });
    });
    connect(chatRoom, &IChatRoom::isDirectChatChanged, ctx, [this, chatRoom]() {
        emitDataChanged(chatRoom,
                        { static_cast<int>(Roles::Name), static_cast<int>(Roles::AvatarPath),
                          static_cast<int>(Roles::HasPresenceState),
                          static_cast<int>(Roles::PresenceState) });
    });
    connect(chatRoom, &IChatRoom::chatUsersChanged, ctx, [this, chatRoom]() {
        emitDataChanged(chatRoom,
                        { static_cast<int>(Roles::Name), static_cast<int>(Roles::AvatarPath),
                          static_cast<int>(Roles::HasPresenceState),
                          static_cast<int>(Roles::PresenceState) });
    });
    connect(chatRoom, &IChatRoom::chatUserRoomStateChanged, ctx, [this, chatRoom]() {
        emitDataChanged(chatRoom,
                        { static_cast<int>(Roles::Name), static_cast<int>(Roles::AvatarPath),
                          static_cast<int>(Roles::HasPresenceState),
                          static_cast<int>(Roles::PresenceState) });
    });

    m_chatRoomContextObjects.insert(chatRoom, ctx);
}

void ChatRoomModel::emitDataChanged(IChatRoom *chatRoom, const QList<int> &roles)
{
    if (!chatRoom) {
        return;
    }
    const auto chatIndex = m_chatProvider->indexOf(chatRoom);
    if (chatIndex >= 0) {
        const auto idx = createIndex(chatIndex, 0);
        Q_EMIT dataChanged(idx, idx, roles);
    }
}

void ChatRoomModel::onChatProviderChanged()
{
    beginResetModel();

    if (m_chatProviderContext) {
        qDeleteAll(m_chatRoomContextObjects);
        m_chatRoomContextObjects.clear();
        m_chatProviderContext->deleteLater();
        m_chatProviderContext = nullptr;
    }

    if (m_chatProvider) {
        m_chatProviderContext = new QObject(this);
        connect(m_chatProvider, &IChatProvider::chatRoomAdded, m_chatProviderContext,
                [this](qsizetype index, IChatRoom *chatRoom, QString) {
                    beginInsertRows(QModelIndex(), index, index);
                    connectChatRoomSignals(chatRoom);
                    endInsertRows();
                });
        connect(m_chatProvider, &IChatProvider::chatRoomRemoved, m_chatProviderContext,
                [this](qsizetype index, IChatRoom *chatRoom) {
                    beginRemoveRows(QModelIndex(), index, index);
                    if (auto *ctx = m_chatRoomContextObjects.take(chatRoom)) {
                        ctx->deleteLater();
                    }
                    endRemoveRows();
                });

        for (qsizetype i = 0, l = m_chatProvider->chatRoomsCount(); i < l; ++i) {
            connectChatRoomSignals(m_chatProvider->chatRoomByIndex(i));
        }
    }

    endResetModel();
}

int ChatRoomModel::rowCount(const QModelIndex &) const
{
    return m_chatProvider ? m_chatProvider->chatRoomsCount() : 0;
}

QVariant ChatRoomModel::data(const QModelIndex &index, int role) const
{
    const auto room = q_check_ptr(m_chatProvider->chatRoomByIndex(index.row()));

    switch (role) {
    case static_cast<int>(Roles::RoomId):
        return room->id();

    case static_cast<int>(Roles::AvatarPath):
        return room->avatarPath();

    case static_cast<int>(Roles::UnreadCount):
        return room->notificationCount();

    case static_cast<int>(Roles::JoinRule):
        return QVariant::fromValue(room->joinRule());

    case static_cast<int>(Roles::IsFavorite):
        return room->isFavorite();

    case static_cast<int>(Roles::HasPresenceState):
        return room->hasPresenceState();

    case static_cast<int>(Roles::PresenceState):
        return QVariant::fromValue(room->presenceState());

    case static_cast<int>(Roles::LatestMessageDate):
        return room->latestMessageDateTime();

    case static_cast<int>(Roles::ChatProvider):
        return QVariant::fromValue(m_chatProvider);

    case static_cast<int>(Roles::Permissions):
        return QVariant::fromValue(room->permissions());

    case static_cast<int>(Roles::OwnJoinState): {
        return QVariant::fromValue(room->ownUserJoinState());
    }

    case static_cast<int>(Roles::TypingUserNames): {
        const auto users = room->typingUsers();
        const auto ownUser = m_chatProvider->userById(m_chatProvider->ownUserId());

        QStringList result;
        result.reserve(users.size());
        for (const auto user : users) {
            if (ownUser && user != ownUser) {
                result.append(user->computedName());
            }
        }

        return result;
    }

    case static_cast<int>(Roles::Name):
    default:
        return room->name();
    }
}
