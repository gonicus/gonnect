#include "AggregatedDirectRoomsOfContact.h"
#include "ChatConnectorManager.h"
#include "FuzzyCompare.h"

AggregatedDirectRoomsOfContact::AggregatedDirectRoomsOfContact(QObject *parent) : QObject{ parent }
{
    connect(this, &AggregatedDirectRoomsOfContact::contactChanged, this,
            &AggregatedDirectRoomsOfContact::onContactChanged);
    connect(this, &AggregatedDirectRoomsOfContact::chatRoomsChanged, this,
            &AggregatedDirectRoomsOfContact::updateBestMatchingRoom);

    auto &chatMan = ChatConnectorManager::instance();
    connect(&chatMan, &ChatConnectorManager::chatConnectorsChanged, this,
            &AggregatedDirectRoomsOfContact::onContactChanged);
}

IChatProvider *AggregatedDirectRoomsOfContact::providerOfRoom(IChatRoom *chatRoom) const
{
    if (!chatRoom) {
        return nullptr;
    }
    const auto providers = ChatConnectorManager::instance().chatConnectors();
    for (auto *provider : providers) {
        if (provider->chatRoomByRoomId(chatRoom->id())) {
            return provider;
        }
    }
    return nullptr;
}

void AggregatedDirectRoomsOfContact::setChatRooms(const QList<IChatRoom *> chatRooms)
{
    if (m_chatRooms != chatRooms) {
        m_chatRooms = chatRooms;
        Q_EMIT chatRoomsChanged();
    }
}

void AggregatedDirectRoomsOfContact::onContactChanged()
{
    if (m_contactConn) {
        QObject::disconnect(m_contactConn);
        m_contactConn = QMetaObject::Connection();
    }

    if (m_contact) {
        m_contactConn = connect(m_contact, &Contact::chatUsersChanged, this,
                                &AggregatedDirectRoomsOfContact::updateChatRooms);
    }

    updateChatRooms();
}

void AggregatedDirectRoomsOfContact::updateChatRooms()
{
    QList<IChatRoom *> chatRooms;

    qDeleteAll(m_providerContextObjects);
    m_providerContextObjects.clear();
    qDeleteAll(m_roomContextObjects);
    m_roomContextObjects.clear();

    if (!m_contact) {
        setChatRooms(chatRooms);
        return;
    }

    const auto providers = ChatConnectorManager::instance().chatConnectors();
    for (auto *provider : providers) {
        processProvider(provider, chatRooms);
    }

    setChatRooms(chatRooms);
}

void AggregatedDirectRoomsOfContact::updateBestMatchingRoom()
{
    IChatRoom *newChatRoom = nullptr;
    const auto &rooms = m_chatRooms;

    if (m_contact) {
        if (rooms.size() == 1) {
            newChatRoom = rooms.at(0);

        } else if (rooms.size()) {
            qreal dist = 0.0;
            const auto contactName = m_contact->name();

            for (auto *room : rooms) {
                const auto newDist = FuzzyCompare::jaroWinklerDistance(contactName, room->name());
                if (newDist > dist) {
                    dist = newDist;
                    newChatRoom = room;
                }
            }

            // Edge case: no room found with fuzzy compare
            if (!newChatRoom) {
                newChatRoom = rooms.at(0);
            }
        }
    }

    if (m_bestRoom != newChatRoom) {
        m_bestRoom = newChatRoom;
        Q_EMIT bestMatchingChatRoomChanged();
    }
}

void AggregatedDirectRoomsOfContact::processProvider(IChatProvider *provider,
                                                     QList<IChatRoom *> &chatRooms)
{
    Q_CHECK_PTR(provider);

    auto ctx = new QObject(this);
    m_providerContextObjects.insert(provider, ctx);

    connect(provider, &IChatProvider::chatRoomAdded, ctx,
            [this](qsizetype, IChatRoom *chatRoom, QString) { addRoom(chatRoom); });

    connect(provider, &IChatProvider::chatRoomRemoved, ctx,
            [this](qsizetype, IChatRoom *chatRoom) { removeRoom(chatRoom); });

    for (qsizetype i = 0, l = provider->chatRoomsCount(); i < l; ++i) {
        auto *room = q_check_ptr(provider->chatRoomByIndex(i));
        processRoom(room, chatRooms);
    }
}

bool AggregatedDirectRoomsOfContact::processRoom(IChatRoom *room, QList<IChatRoom *> &chatRooms)
{
    Q_CHECK_PTR(room);
    const auto chatUsers = m_contact->chatUsers();

    auto ctx = new QObject(this);
    m_roomContextObjects.insert(room, ctx);

    connect(room, &IChatRoom::isDirectChatChanged, ctx, [this, room]() {
        removeRoom(room);
        addRoom(room);
    });
    connect(room, &IChatRoom::otherUserChanged, ctx, [this, room]() {
        removeRoom(room);
        addRoom(room);
    });

    if (room->isDirectChat() && chatUsers.contains(room->otherUser())) {
        chatRooms.append(room);
        return true;
    }
    return false;
}

void AggregatedDirectRoomsOfContact::addRoom(IChatRoom *room)
{
    Q_CHECK_PTR(room);

    QList<IChatRoom *> newList;
    removeRoom(room);
    if (processRoom(room, newList)) {
        auto newChatRooms = m_chatRooms + newList;
        sortChatRoomList(newChatRooms);
        setChatRooms(newChatRooms);
    }
}

void AggregatedDirectRoomsOfContact::removeRoom(IChatRoom *room)
{
    Q_CHECK_PTR(room);

    if (auto *ctx = m_roomContextObjects.take(room)) {
        ctx->deleteLater();
    }

    if (m_chatRooms.contains(room)) {
        QList<IChatRoom *> newChatRooms(m_chatRooms);
        newChatRooms.removeOne(room);
        setChatRooms(newChatRooms);
    }
}

void AggregatedDirectRoomsOfContact::sortChatRoomList(QList<IChatRoom *> &chatRooms) const
{
    std::sort(chatRooms.begin(), chatRooms.end(), [](IChatRoom *a, IChatRoom *b) -> bool {
        if (a->name() != b->name()) {
            return a->name().localeAwareCompare(b->name()) < 0;
        }
        return a->id() < b->id();
    });
}
