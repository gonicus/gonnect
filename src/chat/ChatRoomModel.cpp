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

void ChatRoomModel::onChatProviderChanged()
{
    beginResetModel();

    if (m_chatProviderContext) {
        m_chatProviderContext->deleteLater();
        m_chatProviderContext = nullptr;
    }

    if (m_chatProvider) {
        m_chatProviderContext = new QObject(this);

        connect(m_chatProvider, &IChatProvider::chatRoomAdded, m_chatProviderContext,
                [this](qsizetype index, IChatRoom *, QString) {
                    beginInsertRows(QModelIndex(), index, index);
                    endInsertRows();
                });
        connect(m_chatProvider, &IChatProvider::chatRoomRemoved, m_chatProviderContext,
                [this](qsizetype index, IChatRoom *) {
                    beginRemoveRows(QModelIndex(), index, index);
                    endRemoveRows();
                });
        connect(m_chatProvider, &IChatProvider::chatRoomNameChanged, m_chatProviderContext,
                [this](qsizetype index, IChatRoom *, QString) {
                    const auto idx = createIndex(index, 0);
                    Q_EMIT dataChanged(idx, idx, { static_cast<int>(Roles::Name) });
                });
        connect(m_chatProvider, &IChatProvider::chatRoomAvatarPathChanged, m_chatProviderContext,
                [this](qsizetype index, IChatRoom *, QString) {
                    const auto idx = createIndex(index, 0);
                    Q_EMIT dataChanged(idx, idx, { static_cast<int>(Roles::AvatarPath) });
                });
        connect(m_chatProvider, &IChatProvider::chatRoomIsFavoriteChanged, m_chatProviderContext,
                [this](qsizetype index, IChatRoom *, bool) {
                    const auto idx = createIndex(index, 0);
                    Q_EMIT dataChanged(idx, idx, { static_cast<int>(Roles::IsFavorite) });
                });
        connect(m_chatProvider, &IChatProvider::chatRoomJoinRuleChanged, m_chatProviderContext,
                [this](qsizetype index, IChatRoom *, IChatRoom::JoinRule) {
                    const auto idx = createIndex(index, 0);
                    Q_EMIT dataChanged(idx, idx, { static_cast<int>(Roles::JoinRule) });
                });
        connect(m_chatProvider, &IChatProvider::chatRoomOwnJoinStateChanged, m_chatProviderContext,
                [this](qsizetype index, IChatRoom *, IChatRoom::UserRoomState) {
                    const auto idx = createIndex(index, 0);
                    Q_EMIT dataChanged(idx, idx, { static_cast<int>(Roles::OwnJoinState) });
                });
        connect(m_chatProvider, &IChatProvider::chatRoomNotificationCountChanged,
                m_chatProviderContext, [this](qsizetype index, IChatRoom *, qsizetype) {
                    const auto idx = createIndex(index, 0);
                    Q_EMIT dataChanged(idx, idx, { static_cast<int>(Roles::UnreadCount) });
                });
        connect(m_chatProvider, &IChatProvider::chatRoomLatestActivityChanged,
                m_chatProviderContext, [this](qsizetype index, IChatRoom *, QDateTime) {
                    const auto idx = createIndex(index, 0);
                    Q_EMIT dataChanged(idx, idx, { static_cast<int>(Roles::LatestMessageDate) });
                });
        connect(m_chatProvider, &IChatProvider::chatRoomPermissionsChanged, m_chatProviderContext,
                [this](qsizetype index, IChatRoom *, IChatRoom::Permissions) {
                    const auto idx = createIndex(index, 0);
                    Q_EMIT dataChanged(idx, idx, { static_cast<int>(Roles::Permissions) });
                });
        connect(m_chatProvider, &IChatProvider::chatRoomTypingChanged, m_chatProviderContext,
                [this](qsizetype index, IChatRoom *) {
                    const auto idx = createIndex(index, 0);
                    Q_EMIT dataChanged(idx, idx, { static_cast<int>(Roles::TypingUserNames) });
                });
        connect(m_chatProvider, &IChatProvider::chatUserPropertiesChanged, m_chatProviderContext,
                [this](ChatUser *, IChatRoom *chatRoom, qsizetype index) {
                    if (chatRoom) {
                        const auto idx = createIndex(index, 0);
                        Q_EMIT dataChanged(idx, idx,
                                           { static_cast<int>(Roles::Name),
                                             static_cast<int>(Roles::AvatarPath),
                                             static_cast<int>(Roles::HasPresenceState),
                                             static_cast<int>(Roles::PresenceState) });
                    }
                });
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
