#include "ChatRoomUsers.h"

ChatRoomUsers::ChatRoomUsers(QObject *parent) : QAbstractListModel{ parent }
{

    connect(this, &ChatRoomUsers::chatRoomChanged, this, [this]() {
        beginResetModel();
        endResetModel();
    });
}

QHash<int, QByteArray> ChatRoomUsers::roleNames() const
{
    return {
        { static_cast<int>(Roles::Id), "id" },
        { static_cast<int>(Roles::ComputedName), "computedName" },
        { static_cast<int>(Roles::AvatarPath), "avatarPath" },
    };
}

int ChatRoomUsers::rowCount(const QModelIndex &) const
{
    return m_chatRoom ? m_chatRoom->chatUserCount() : 0;
}

QVariant ChatRoomUsers::data(const QModelIndex &index, int role) const
{
    if (!m_chatRoom || !index.isValid()) {
        return QVariant();
    }

    const auto user = q_check_ptr(m_chatRoom->chatUsers().at(index.row()));

    switch (role) {
    case static_cast<int>(Roles::Id):
        return user->id();

    case static_cast<int>(Roles::AvatarPath):
        return user->avatarPath();

    case static_cast<int>(Roles::ComputedName):
    default:
        return user->computedName();
    }
}
