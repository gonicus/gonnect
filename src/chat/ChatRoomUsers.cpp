#include "ChatRoomUsers.h"
#include "AddressBook.h"

ChatRoomUsers::ChatRoomUsers(QObject *parent) : QAbstractListModel{ parent }
{
    connect(this, &ChatRoomUsers::chatRoomChanged, this, [this]() {
        beginResetModel();

        if (m_chatRoomContext) {
            m_chatRoomContext->deleteLater();
            m_chatRoomContext = nullptr;
        }
        m_avatarSignaledUsers.clear();

        if (m_chatRoom) {
            m_chatRoomContext = new QObject(this);
            connect(&AddressBook::instance(), &AddressBook::chatUserMappingAdded, m_chatRoomContext,
                    [this](ChatUser *user) { refreshAvatarPath(user); });

            const auto users = std::as_const(m_chatRoom->chatUsers());
            for (auto *user : users) {
                connectUserAvatarSignals(user);
            }
            connect(m_chatRoom, &IChatRoom::chatUsersChanged, m_chatRoomContext, [this]() {
                const auto users = std::as_const(m_chatRoom->chatUsers());
                for (auto *user : users) {
                    connectUserAvatarSignals(user);
                }
            });
        }

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

    case static_cast<int>(Roles::AvatarPath): {
        if (const auto *contact = AddressBook::instance().lookupByChatUser(user)) {
            return contact->avatarPath();
        }
        return user->avatarPath();
    }

    case static_cast<int>(Roles::ComputedName):
    default:
        return user->computedName();
    }
}

void ChatRoomUsers::connectUserAvatarSignals(ChatUser *user)
{
    if (m_avatarSignaledUsers.contains(user)) {
        return;
    }
    m_avatarSignaledUsers.insert(user);

    connect(user, &ChatUser::avatarPathChanged, m_chatRoomContext,
            [this, user]() { refreshAvatarPath(user); });
}

void ChatRoomUsers::refreshAvatarPath(ChatUser *user)
{
    if (!m_chatRoom) {
        return;
    }
    const auto row = m_chatRoom->chatUsers().indexOf(user);
    if (row < 0) {
        return;
    }
    const auto idx = createIndex(row, 0);
    Q_EMIT dataChanged(idx, idx, { static_cast<int>(Roles::AvatarPath) });
}
