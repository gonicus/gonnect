#include "ChatRoomModel.h"
#include "IChatProvider.h"
#include "IChatRoom.h"

ChatRoomModel::ChatRoomModel(QObject *parent) : QAbstractListModel{ parent }
{
    connect(this, &ChatRoomModel::chatProviderChanged, this, &ChatRoomModel::onChatProviderChanged);
}

QHash<int, QByteArray> ChatRoomModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::RoomId), "roomId" },
        { static_cast<int>(Roles::Name), "name" },
        { static_cast<int>(Roles::UnreadCount), "unreadCount" },
    };
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
                [this](qsizetype index, IChatRoom *) {
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
                    emit dataChanged(idx, idx, { static_cast<int>(Roles::Name) });
                });
        connect(m_chatProvider, &IChatProvider::chatRoomNotificationCountChanged,
                m_chatProviderContext, [this](qsizetype index, IChatRoom *, qsizetype) {
                    const auto idx = createIndex(index, 0);
                    emit dataChanged(idx, idx, { static_cast<int>(Roles::UnreadCount) });
                });
    }

    endResetModel();
}

int ChatRoomModel::rowCount(const QModelIndex &) const
{
    return m_chatProvider ? m_chatProvider->chatRooms().size() : 0;
}

QVariant ChatRoomModel::data(const QModelIndex &index, int role) const
{
    const auto room = q_check_ptr(m_chatProvider->chatRooms().at(index.row()));

    switch (role) {
    case static_cast<int>(Roles::RoomId):
        return room->id();

    case static_cast<int>(Roles::UnreadCount):
        return room->notificationCount();

    case static_cast<int>(Roles::Name):
    default:
        return room->name();
    }
}
