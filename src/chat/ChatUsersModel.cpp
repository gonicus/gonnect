#include "ChatUsersModel.h"
#include "ChatUser.h"

ChatUsersModel::ChatUsersModel(QObject *parent) : QAbstractListModel{ parent }
{

    connect(this, &ChatUsersModel::chatProviderChanged, this, [this]() {
        beginResetModel();

        if (m_chatProviderContext) {
            m_chatProviderContext->deleteLater();
            m_chatProviderContext = nullptr;
        }

        if (m_chatProvider) {
            m_chatProviderContext = new QObject(this);

            connect(m_chatProvider, &IChatProvider::userAdded, m_chatProviderContext,
                    [this](QString, ChatUser *, qsizetype index) {
                        beginInsertRows(QModelIndex(), index, index);
                        endInsertRows();
                    });

            connect(m_chatProvider, &IChatProvider::userRemoved, m_chatProviderContext,
                    [this](QString, ChatUser *, qsizetype index) {
                        beginRemoveRows(QModelIndex(), index, index);
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

    case static_cast<int>(Roles::AvatarPath):
        return user->avatarPath();

    case static_cast<int>(Roles::HasPresenceState):
        return user->hasPresenceState();

    case static_cast<int>(Roles::PresenceState):
        return QVariant::fromValue(user->presenceState());

    case static_cast<int>(Roles::Name):
    default:
        return user->computedName();
    }
}
