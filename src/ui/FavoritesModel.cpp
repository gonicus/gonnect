#include "FavoritesModel.h"
#include "NumberStats.h"
#include "NumberStat.h"
#include "Contact.h"
#include "AddressBook.h"
#include "SIPCallManager.h"
#include "ChatConnectorManager.h"

FavoritesModel::FavoritesModel(QObject *parent) : QAbstractListModel{ parent }
{
    m_modelUpdateTimer.setSingleShot(true);
    m_modelUpdateTimer.setInterval(50);
    m_modelUpdateTimer.callOnTimeout(this, &FavoritesModel::updateModel);

    auto &numStats = NumberStats::instance();
    connect(&numStats, &NumberStats::modelReset, this, &FavoritesModel::updateModel);
    connect(&numStats, &NumberStats::favoriteAdded, this, &FavoritesModel::scheduleModelUpdate);
    connect(&numStats, &NumberStats::favoriteRemoved, this, &FavoritesModel::updateModel);

    connect(&SIPCallManager::instance(), &SIPCallManager::blocksChanged, this,
            &FavoritesModel::scheduleModelUpdate);

    connect(&ChatConnectorManager::instance(), &ChatConnectorManager::chatConnectorsChanged, this,
            &FavoritesModel::scheduleModelUpdate);

    connect(&AddressBook::instance(), &AddressBook::contactsReady, this,
            &FavoritesModel::scheduleModelUpdate);
    connect(&AddressBook::instance(), &AddressBook::contactsCleared, this,
            &FavoritesModel::scheduleModelUpdate);
    connect(&AddressBook::instance(), &AddressBook::contactAdded, this,
            &FavoritesModel::scheduleModelUpdate);
    connect(&AddressBook::instance(), &AddressBook::contactModified, this,
            &FavoritesModel::scheduleModelUpdate);
    connect(&AddressBook::instance(), &AddressBook::contactRemoved, this,
            [this](QString contactId) {
                for (auto &favEntry : std::as_const(m_favorites)) {
                    if (favEntry->contact && favEntry->contact->id() == contactId) {
                        m_favoriteContactLookup.remove(favEntry->contact);
                        favEntry->contact = nullptr;
                        scheduleModelUpdate();
                        return;
                    }
                }
            });

    updateModel();
}

QHash<int, QByteArray> FavoritesModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::Name), "name" },
        { static_cast<int>(Roles::Company), "company" },
        { static_cast<int>(Roles::HasBuddyState), "hasBuddyState" },
        { static_cast<int>(Roles::HasAvatar), "hasAvatar" },
        { static_cast<int>(Roles::AvatarPath), "avatarPath" },
        { static_cast<int>(Roles::ChatProvider), "chatProvider" },
        { static_cast<int>(Roles::ChatRoom), "chatRoom" },
        { static_cast<int>(Roles::Addresses), "addresses" },
        { static_cast<int>(Roles::SubscribableNumber), "subscribableNumber" },
    };
}

void FavoritesModel::updateModel()
{
    if (m_isUpdating) {
        return;
    }
    m_isUpdating = true;
    beginResetModel();

    m_favoriteContactLookup.clear();
    m_favorites.clear();

    // Favorites from database
    const auto dbFavs = NumberStats::instance().favorites();
    for (const auto *dbFav : dbFavs) {
        FavoriteEntry *entry = nullptr;
        if (dbFav->contact) {
            entry = m_favoriteContactLookup.value(dbFav->contact, nullptr);
        }

        // Initialize entry
        if (!entry) {
            auto uniqueEntry = std::make_unique<FavoriteEntry>();
            entry = uniqueEntry.get();
            entry->contact = dbFav->contact;

            if (entry->contact) {
                m_favoriteContactLookup.insert(entry->contact, entry);
            }
            m_favorites.push_back(std::move(uniqueEntry));
        }

        // Add new address
        auto addr = std::make_unique<FavoriteEntry::Addr>();
        addr->addr = dbFav->phoneNumber;
        addr->contactType = dbFav->contactType;

        if (entry->contact) {
            const auto numObj = entry->contact->phoneNumberObject(dbFav->phoneNumber);
            if (numObj.number == dbFav->phoneNumber) {
                addr->numberType = numObj.type;
                addr->isSubscribable = numObj.isSipSubscriptable;
            }
        }

        entry->addrs.push_back(std::move(addr));
    }

    // Favorites from chat providers
    if (m_chatProviderContext) {
        m_chatProviderContext->deleteLater();
        m_chatProviderContext = nullptr;
    }

    const auto providers = ChatConnectorManager::instance().chatConnectors();
    if (!providers.isEmpty()) {
        m_chatProviderContext = new QObject(this);
    }

    for (auto provider : providers) {
        addChatProviderSignals(*q_check_ptr(provider));

        for (qsizetype i = 0, l = provider->chatRoomsCount(); i < l; ++i) {
            auto *room = provider->chatRoomByIndex(i);
            if (!room->isFavorite()) {
                continue;
            }
            Contact *contact = nullptr;

            if (const ChatUser *otherUser = room->otherUser()) {
                contact = AddressBook::instance().lookupByChatUser(otherUser);
            }

            FavoriteEntry *entry = nullptr;
            if (contact) {
                entry = m_favoriteContactLookup.value(contact, nullptr);
            }

            if (!entry) {
                auto favEntry = std::make_unique<FavoriteEntry>();
                favEntry->contact = contact;
                entry = favEntry.get();

                if (favEntry->contact) {
                    m_favoriteContactLookup.insert(favEntry->contact, entry);
                }
                m_favorites.push_back(std::move(favEntry));
            }

            entry->chatProvider = provider;
            entry->chatRoom = room;

            auto addr = std::make_unique<FavoriteEntry::Addr>();
            addr->contactType = NumberStats::ContactType::ChatRoomId;
            addr->addr = room->id();
            entry->addrs.push_back(std::move(addr));
        }
    }

    sortInnerModel();

    endResetModel();
    m_isUpdating = false;
}

void FavoritesModel::sortInnerModel()
{
    // Sort m_favorites
    std::sort(m_favorites.begin(), m_favorites.end(),
              [](const std::unique_ptr<FavoriteEntry> &a, const std::unique_ptr<FavoriteEntry> &b)
                      -> bool { return a->name().localeAwareCompare(b->name()) < 0; });

    // Sort addresses inside FavoriteEntry
    for (auto &favEntry : std::as_const(m_favorites)) {
        std::sort(favEntry->addrs.begin(), favEntry->addrs.end(),
                  [](const std::unique_ptr<FavoriteEntry::Addr> &a,
                     const std::unique_ptr<FavoriteEntry::Addr> &b) -> bool {
                      static const QHash<NumberStats::ContactType, quint8> contactTypeSortOrder = {
                          { NumberStats::ContactType::PhoneNumber, 1 },
                          { NumberStats::ContactType::JitsiMeetUrl, 2 },
                          { NumberStats::ContactType::ChatRoomId, 3 },
                      };
                      static const QHash<Contact::NumberType, quint8> numberTypeOrder = {
                          { Contact::NumberType::Unknown, 1 },
                          { Contact::NumberType::Commercial, 2 },
                          { Contact::NumberType::Mobile, 3 },
                          { Contact::NumberType::Home, 4 },
                      };

                      const auto contactSortA = contactTypeSortOrder.value(a->contactType);
                      const auto contactSortB = contactTypeSortOrder.value(b->contactType);

                      if (contactSortA == contactSortB) {
                          return numberTypeOrder.value(a->numberType)
                                  < numberTypeOrder.value(b->numberType);
                      }
                      return contactSortA < contactSortB;
                  });
    }
}

void FavoritesModel::addChatProviderSignals(IChatProvider &provider)
{
    connect(&provider, &IChatProvider::chatRoomAdded, m_chatProviderContext,
            [this](qsizetype, IChatRoom *chatRoom, QString) {
                if (chatRoom->isFavorite()) {
                    scheduleModelUpdate();
                }
            });

    connect(&provider, &IChatProvider::chatRoomRemoved, m_chatProviderContext,
            [this](qsizetype, IChatRoom *chatRoom) {
                if (chatRoom->isFavorite()) {
                    scheduleModelUpdate();
                }
            });

    connect(&provider, &IChatProvider::chatRoomIsFavoriteChanged, m_chatProviderContext,
            [this](qsizetype, IChatRoom *, bool) { scheduleModelUpdate(); });

    connect(&provider, &IChatProvider::chatRoomNameChanged, m_chatProviderContext,
            [this](qsizetype, IChatRoom *chatRoom, QString) {
                if (chatRoom->isFavorite()) {
                    scheduleModelUpdate();
                }
            });

    connect(&provider, &IChatProvider::chatRoomAvatarPathChanged, m_chatProviderContext,
            [this](qsizetype, IChatRoom *chatRoom, QString) {
                if (chatRoom->isFavorite()) {
                    for (std::size_t i = 0; i < m_favorites.size(); ++i) {
                        if (chatRoom == m_favorites.at(i)->chatRoom) {
                            const auto idx = createIndex(i, 0);
                            Q_EMIT dataChanged(idx, idx,
                                               { static_cast<int>(Roles::HasAvatar),
                                                 static_cast<int>(Roles::AvatarPath) });
                        }
                    }
                }
            });
}

int FavoritesModel::rowCount(const QModelIndex &) const
{
    return m_favorites.size();
}

QVariant FavoritesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    const auto &favEntry = m_favorites.at(index.row());

    switch (role) {

    case static_cast<int>(Roles::Company):
        return favEntry->contact ? favEntry->contact->company() : QString();

    case static_cast<int>(Roles::HasBuddyState):
        return favEntry->contact && !favEntry->contact->subscriptableNumber().isEmpty();

    case static_cast<int>(Roles::SubscribableNumber):
        return favEntry->contact ? favEntry->contact->subscriptableNumber() : QString();

    case static_cast<int>(Roles::HasAvatar):
        return favEntry->contact && favEntry->contact->hasAvatar();

    case static_cast<int>(Roles::AvatarPath):
        return favEntry->contact && favEntry->contact->hasAvatar() ? favEntry->contact->avatarPath()
                                                                   : QString();

    case static_cast<int>(Roles::ChatProvider):
        return QVariant::fromValue(favEntry->chatProvider);

    case static_cast<int>(Roles::ChatRoom):
        return QVariant::fromValue(favEntry->chatRoom);

    case static_cast<int>(Roles::Addresses): {
        QVariantList l;

        for (const auto &addr : std::as_const(favEntry->addrs)) {
            QVariantMap m;
            m["addr"] = addr->addr;
            m["contactType"] = static_cast<int>(addr->contactType);
            m["numberType"] = static_cast<int>(addr->numberType);
            l.append(m);
        }
        return l;
    }

    case static_cast<int>(Roles::Name):
    default:
        return favEntry->name();
    }
}

void FavoritesModel::scheduleModelUpdate()
{
    m_modelUpdateTimer.start();
}

QString FavoriteEntry::name() const
{
    if (contact && !contact->name().isEmpty()) {
        return contact->name();
    }

    for (const auto &addr : std::as_const(addrs)) {
        if (!addr->addr.isEmpty()) {
            return addr->addr;
        }
    }

    if (chatRoom) {
        return chatRoom->name();
    }

    return QString();
}
