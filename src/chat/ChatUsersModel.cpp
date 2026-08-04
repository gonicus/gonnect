#include "ChatUsersModel.h"
#include "ChatUser.h"
#include "AddressBook.h"
#include "AvatarPrioHelper.h"

ChatUsersModel::ChatUsersModel(QObject *parent) : QAbstractListModel{ parent }
{

    connect(this, &ChatUsersModel::chatProviderChanged, this, [this]() {
        beginResetModel();

        if (m_chatProviderContext) {
            m_chatProviderContext->deleteLater();
            m_chatProviderContext = nullptr;
            m_avatarSignaledUsers.clear();
        }

        if (m_chatProvider) {
            m_chatProviderContext = new QObject(this);

            connect(&AddressBook::instance(), &AddressBook::chatUserMappingAdded,
                    m_chatProviderContext, [this](ChatUser *user) { refreshAvatarPath(user); });
            connect(&AvatarPrioHelper::instance(), &AvatarPrioHelper::priosChanged,
                    m_chatProviderContext, [this]() {
                        const auto rows = rowCount(QModelIndex());
                        if (rows > 0) {
                            Q_EMIT dataChanged(createIndex(0, 0), createIndex(rows - 1, 0),
                                               { static_cast<int>(Roles::AvatarPath) });
                        }
                    });

            connect(m_chatProvider, &IChatProvider::userAdded, m_chatProviderContext,
                    [this](QString, ChatUser *user, qsizetype index) {
                        beginInsertRows(QModelIndex(), index, index);
                        endInsertRows();
                        connectUserAvatarSignals(user);
                    });

            connect(m_chatProvider, &IChatProvider::userRemoved, m_chatProviderContext,
                    [this](QString, ChatUser *chatUser, qsizetype index) {
                        beginRemoveRows(QModelIndex(), index, index);
                        m_avatarSignaledUsers.remove(chatUser);
                        endRemoveRows();
                    });

            connect(m_chatProvider, &IChatProvider::chatUserPropertiesChanged,
                    m_chatProviderContext, [this](ChatUser *, IChatRoom *, qsizetype index) {
                        const auto idx = createIndex(index, 0);
                        Q_EMIT dataChanged(idx, idx,
                                           { static_cast<int>(Roles::Name),
                                             static_cast<int>(Roles::AvatarPath),
                                             static_cast<int>(Roles::HasPresenceState),
                                             static_cast<int>(Roles::PresenceState) });
                    });

            for (qsizetype i = 0, l = m_chatProvider->chatRoomsCount(); i < l; ++i) {
                const auto room = m_chatProvider->chatRoomByIndex(i);
                const auto &users = room->chatUsers();
                for (auto *user : users) {
                    connectUserAvatarSignals(user);
                }
            }
        }

        endResetModel();
    });
}

QHash<int, QByteArray> ChatUsersModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::Id), "id" },
        { static_cast<int>(Roles::Name), "name" },
        { static_cast<int>(Roles::AvatarPath), "avatarPath" },
        { static_cast<int>(Roles::HasPresenceState), "hasPresenceState" },
        { static_cast<int>(Roles::PresenceState), "presenceState" },
    };
}

int ChatUsersModel::rowCount(const QModelIndex &) const
{
    return m_chatProvider ? m_chatProvider->users().size() : 0;
}

QVariant ChatUsersModel::data(const QModelIndex &index, int role) const
{
    if (!m_chatProvider) {
        return QVariant();
    }

    auto user = q_check_ptr(m_chatProvider->users().at(index.row()));

    switch (role) {
    case static_cast<int>(Roles::Id):
        return user->id();

    case static_cast<int>(Roles::AvatarPath): {
        if (const auto *contact = AddressBook::instance().lookupByChatUser(user)) {
            return contact->avatarPath();
        }
        return user->avatarPath();
    }

    case static_cast<int>(Roles::HasPresenceState):
        return user->hasPresenceState();

    case static_cast<int>(Roles::PresenceState):
        return QVariant::fromValue(user->presenceState());

    case static_cast<int>(Roles::Name):
    default:
        return user->computedName();
    }
}

void ChatUsersModel::connectUserAvatarSignals(ChatUser *user)
{
    if (m_avatarSignaledUsers.contains(user)) {
        return;
    }
    m_avatarSignaledUsers.insert(user);

    connect(user, &ChatUser::avatarPathChanged, m_chatProviderContext,
            [this, user]() { refreshAvatarPath(user); });
}

void ChatUsersModel::refreshAvatarPath(ChatUser *user)
{
    const auto row = m_chatProvider->users().indexOf(user);
    if (row < 0) {
        return;
    }
    const auto idx = createIndex(row, 0);
    Q_EMIT dataChanged(idx, idx, { static_cast<int>(Roles::AvatarPath) });
}
