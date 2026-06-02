#include "FavoritesModel.h"
#include "NumberStats.h"
#include "NumberStat.h"
#include "Contact.h"
#include "PhoneNumberUtil.h"
#include "SIPCallManager.h"
#include "ChatConnectorManager.h"

FavoritesModel::FavoritesModel(QObject *parent) : QAbstractListModel{ parent }
{
    auto &numStats = NumberStats::instance();
    connect(&numStats, &NumberStats::favoriteAdded, this, &FavoritesModel::updateModel);
    connect(&numStats, &NumberStats::favoriteRemoved, this, &FavoritesModel::updateModel);
    connect(&numStats, &NumberStats::modelReset, this, &FavoritesModel::updateModel);

    connect(&SIPCallManager::instance(), &SIPCallManager::blocksChanged, this,
            &FavoritesModel::updateModel);

    connect(&ChatConnectorManager::instance(), &ChatConnectorManager::chatConnectorsChanged, this,
            &FavoritesModel::updateModel);

    updateModel();
}

QHash<int, QByteArray> FavoritesModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::PhoneNumber), "phoneNumber" },
        { static_cast<int>(Roles::Name), "name" },
        { static_cast<int>(Roles::Company), "company" },
        { static_cast<int>(Roles::HasBuddyState), "hasBuddyState" },
        { static_cast<int>(Roles::HasAvatar), "hasAvatar" },
        { static_cast<int>(Roles::AvatarPath), "avatarPath" },
        { static_cast<int>(Roles::NumberType), "numberType" },
        { static_cast<int>(Roles::ContactType), "contactType" },
        { static_cast<int>(Roles::ChatProvider), "chatProvider" },
    };
}

void FavoritesModel::updateModel()
{
    beginResetModel();

    // Favorites from database
    m_favorites = NumberStats::instance().favorites();

    // Favorites from chat providers
    if (m_chatProviderContext) {
        m_chatProviderContext->deleteLater();
        m_chatProviderContext = nullptr;
    }

    m_favoriteChatRooms.clear();
    const auto providers = ChatConnectorManager::instance().chatConnectors();

    if (!providers.isEmpty()) {
        m_chatProviderContext = new QObject(this);
    }

    for (auto provider : providers) {
        const auto rooms = provider->chatRooms();

        connect(provider, &IChatProvider::chatRoomAdded, m_chatProviderContext,
                [this, provider](qsizetype, IChatRoom *chatRoom, QString) {
                    if (chatRoom->isFavorite()) {
                        const auto idx = m_favorites.size() + m_favoriteChatRooms.size();
                        beginInsertRows(QModelIndex(), idx, idx);
                        m_favoriteChatRooms.append({ provider, chatRoom });
                        endInsertRows();
                    }
                });

        connect(provider, &IChatProvider::chatRoomRemoved, m_chatProviderContext,
                [this](qsizetype, IChatRoom *chatRoom) {
                    for (qsizetype i = 0; i < m_favoriteChatRooms.size(); ++i) {
                        if (m_favoriteChatRooms.at(i).room == chatRoom) {
                            const auto idx = m_favorites.size() + i;
                            beginRemoveRows(QModelIndex(), idx, idx);
                            m_favoriteChatRooms.removeAt(idx);
                            endRemoveRows();
                            return;
                        }
                    }
                });

        connect(provider, &IChatProvider::chatRoomIsFavoriteChanged, m_chatProviderContext,
                [this, provider](qsizetype, IChatRoom *chatRoom, bool isFavorite) {
                    if (isFavorite) {
                        for (const auto favRoom : std::as_const(m_favoriteChatRooms)) {
                            if (favRoom.room == chatRoom) {
                                return;
                            }
                        }

                        const auto idx = m_favorites.size() + m_favoriteChatRooms.size();
                        beginInsertRows(QModelIndex(), idx, idx);
                        m_favoriteChatRooms.append({ provider, chatRoom });
                        endInsertRows();

                    } else {
                        for (qsizetype i = 0; i < m_favoriteChatRooms.size(); ++i) {
                            if (m_favoriteChatRooms.at(i).room == chatRoom) {
                                const auto idx = m_favorites.size() + i;
                                beginRemoveRows(QModelIndex(), idx, idx);
                                m_favoriteChatRooms.removeAt(i);
                                endRemoveRows();
                                return;
                            }
                        }
                    }
                });

        connect(provider, &IChatProvider::chatRoomNameChanged, m_chatProviderContext,
                [this](qsizetype, IChatRoom *chatRoom, QString) {
                    if (chatRoom->isFavorite()) {
                        for (qsizetype i = 0; i < m_favoriteChatRooms.size(); ++i) {
                            if (chatRoom == m_favoriteChatRooms.at(i).room) {
                                const auto idx = createIndex(m_favorites.size() + i, 0);
                                Q_EMIT dataChanged(idx, idx, { static_cast<int>(Roles::Name) });
                            }
                        }
                    }
                });

        connect(provider, &IChatProvider::chatRoomAvatarPathChanged, m_chatProviderContext,
                [this](qsizetype, IChatRoom *chatRoom, QString) {
                    if (chatRoom->isFavorite()) {
                        for (qsizetype i = 0; i < m_favoriteChatRooms.size(); ++i) {
                            if (chatRoom == m_favoriteChatRooms.at(i).room) {
                                const auto idx = createIndex(m_favorites.size() + i, 0);
                                Q_EMIT dataChanged(idx, idx,
                                                   { static_cast<int>(Roles::HasAvatar),
                                                     static_cast<int>(Roles::AvatarPath) });
                            }
                        }
                    }
                });

        for (auto room : rooms) {
            if (room->isFavorite()) {
                m_favoriteChatRooms.append({ provider, room });
            }
        }
    }

    endResetModel();
}

int FavoritesModel::rowCount(const QModelIndex &) const
{
    return m_favorites.size() + m_favoriteChatRooms.size();
}

QVariant FavoritesModel::data(const QModelIndex &index, int role) const
{
    const auto row = index.row();

    if (row < m_favorites.size()) {
        // Row is in m_favorites

        const auto item = m_favorites.at(row);
        ContactInfo contactInfo =
                PhoneNumberUtil::instance().contactInfoBySipUrl(item->phoneNumber);

        switch (role) {
        case static_cast<int>(Roles::PhoneNumber):
            return item->phoneNumber;

        case static_cast<int>(Roles::Company):
            return item->contact ? item->contact->company() : "";

        case static_cast<int>(Roles::HasBuddyState):
            return item->contact
                    ? item->contact->phoneNumberObject(item->phoneNumber).isSipSubscriptable
                    : false;

        case static_cast<int>(Roles::NumberType):
            return static_cast<int>(
                    item->contact ? item->contact->phoneNumberObject(item->phoneNumber).type
                                  : Contact::NumberType::Unknown);

        case static_cast<int>(Roles::HasAvatar):
            return item->contact && item->contact->hasAvatar();

        case static_cast<int>(Roles::AvatarPath):
            return (item->contact && item->contact->hasAvatar()) ? item->contact->avatarPath() : "";

        case static_cast<int>(Roles::ContactType):
            return static_cast<int>(item->contactType);

        case static_cast<int>(Roles::ChatProvider):
            return QVariant::fromValue(nullptr);

        case static_cast<int>(Roles::Name):
        default:
            return item->contact ? item->contact->name() : "";
        }

    } else {
        // Row is in m_favoriteChatRooms

        const auto &item = m_favoriteChatRooms.at(row - m_favorites.length());

        switch (role) {
        case static_cast<int>(Roles::PhoneNumber):
            return item.room->id();

        case static_cast<int>(Roles::Company):
            return item.provider->displayName();

        case static_cast<int>(Roles::HasBuddyState):
            return false;

        case static_cast<int>(Roles::NumberType):
            return QVariant::fromValue(Contact::NumberType::Unknown);

        case static_cast<int>(Roles::HasAvatar):
            return !item.room->avatarPath().isEmpty();

        case static_cast<int>(Roles::AvatarPath):
            return item.room->avatarPath();

        case static_cast<int>(Roles::ContactType):
            return QVariant::fromValue(NumberStats::ContactType::ChatRoomId);

        case static_cast<int>(Roles::ChatProvider):
            return QVariant::fromValue(item.provider);

        case static_cast<int>(Roles::Name):
        default:
            return item.room->name();
        }
    }
}
