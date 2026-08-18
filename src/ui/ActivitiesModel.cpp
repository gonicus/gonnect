#include "ActivitiesModel.h"

#include "AddressBook.h"
#include "AvatarManager.h"
#include "CallHistory.h"
#include "CallHistoryItem.h"
#include "ChatConnectorManager.h"
#include "ChatMessage.h"
#include "ChatMessageContentAudioFile.h"
#include "ChatMessageContentFile.h"
#include "ChatMessageContentImage.h"
#include "ChatMessageContentText.h"
#include "ChatMessageContentVideoFile.h"
#include "ChatUser.h"
#include "IChatProvider.h"
#include "IChatRoom.h"
#include "NumberStat.h"
#include "NumberStats.h"
#include "PhoneNumberUtil.h"
#include "SIPCallManager.h"

namespace {

QString chatMessagePreview(const ChatMessage *message)
{
    const auto content = message->content();

    if (const auto textContent = qobject_cast<ChatMessageContentText *>(content)) {
        return textContent->simpleText();
    }
    if (qobject_cast<ChatMessageContentImage *>(content)) {
        return ActivitiesModel::tr("[Image]");
    }
    if (qobject_cast<ChatMessageContentAudioFile *>(content)) {
        return ActivitiesModel::tr("[Audio]");
    }
    if (qobject_cast<ChatMessageContentVideoFile *>(content)) {
        return ActivitiesModel::tr("[Video]");
    }
    if (qobject_cast<ChatMessageContentFile *>(content)) {
        return ActivitiesModel::tr("[File]");
    }

    return ActivitiesModel::tr("[Message]");
}

} // namespace

ActivitiesModel::ActivitiesModel(QObject *parent) : QAbstractListModel{ parent }
{
    populateCallHistory();

    auto &history = CallHistory::instance();
    connect(&history, &CallHistory::itemAdded, this, &ActivitiesModel::handleCallItemAdded);
    connect(&history, &CallHistory::dataChanged, this, &ActivitiesModel::handleCallItemChanged);

    auto &manager = ChatConnectorManager::instance();
    connect(&manager, &ChatConnectorManager::chatConnectorsChanged, this,
            &ActivitiesModel::subscribeToProviders);
    subscribeToProviders();

    connect(&AvatarManager::instance(), &AvatarManager::avatarsLoaded, this, [this]() {
        refreshCallRoles(
                { static_cast<int>(Roles::HasAvatar), static_cast<int>(Roles::AvatarPath) });
    });
    connect(&AvatarManager::instance(), &AvatarManager::avatarAdded, this, [this](QString) {
        refreshCallRoles(
                { static_cast<int>(Roles::HasAvatar), static_cast<int>(Roles::AvatarPath) });
    });
    connect(&AvatarManager::instance(), &AvatarManager::avatarRemoved, this, [this](QString) {
        refreshCallRoles(
                { static_cast<int>(Roles::HasAvatar), static_cast<int>(Roles::AvatarPath) });
    });

    connect(&SIPCallManager::instance(), &SIPCallManager::blocksChanged, this,
            [this]() { refreshCallRoles({ static_cast<int>(Roles::IsBlocked) }); });

    const auto &numStats = NumberStats::instance();
    connect(&numStats, &NumberStats::favoriteAdded, this,
            [this]() { refreshCallRoles({ static_cast<int>(Roles::IsFavorite) }); });
    connect(&numStats, &NumberStats::favoriteRemoved, this,
            [this]() { refreshCallRoles({ static_cast<int>(Roles::IsFavorite) }); });

    connect(this, &ActivitiesModel::limitChanged, this, &ActivitiesModel::trimToLimit);

    connect(&AddressBook::instance(), &AddressBook::contactsReady, this, [this]() {
        beginResetModel();
        endResetModel();
    });

    const auto trackContactAvatar = [this](Contact *contact) {
        if (m_avatarTrackedContacts.contains(contact)) {
            return;
        }
        m_avatarTrackedContacts.insert(contact);
        connect(contact, &QObject::destroyed, this,
                [this, contact]() { m_avatarTrackedContacts.remove(contact); });
        connect(contact, &Contact::avatarChanged, this, [this]() {
            refreshCallRoles(
                    { static_cast<int>(Roles::HasAvatar), static_cast<int>(Roles::AvatarPath) });
        });
    };
    connect(&AddressBook::instance(), &AddressBook::contactAdded, this, trackContactAvatar);
    connect(&AddressBook::instance(), &AddressBook::contactModified, this, trackContactAvatar);
}

QHash<int, QByteArray> ActivitiesModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::Id), "id" },
        { static_cast<int>(Roles::Day), "day" },
        { static_cast<int>(Roles::Time), "time" },
        { static_cast<int>(Roles::Kind), "kind" },
        { static_cast<int>(Roles::IsSIPCall), "isSIPCall" },
        { static_cast<int>(Roles::IsJitsiMeetCall), "isJitsiMeetCall" },
        { static_cast<int>(Roles::IsChatMessage), "isChatMessage" },
        { static_cast<int>(Roles::Title), "title" },
        { static_cast<int>(Roles::Subtitle), "subtitle" },
        { static_cast<int>(Roles::Text), "text" },
        { static_cast<int>(Roles::Location), "location" },
        { static_cast<int>(Roles::HasAvatar), "hasAvatar" },
        { static_cast<int>(Roles::AvatarPath), "avatarPath" },
        { static_cast<int>(Roles::Account), "account" },
        { static_cast<int>(Roles::ContactId), "contactId" },
        { static_cast<int>(Roles::RemoteUrl), "remoteUrl" },
        { static_cast<int>(Roles::RemotePhoneNumber), "remotePhoneNumber" },
        { static_cast<int>(Roles::DurationSeconds), "durationSeconds" },
        { static_cast<int>(Roles::WasEstablished), "wasEstablished" },
        { static_cast<int>(Roles::IsAnonymous), "isAnonymous" },
        { static_cast<int>(Roles::IsFavorite), "isFavorite" },
        { static_cast<int>(Roles::IsBlocked), "isBlocked" },
        { static_cast<int>(Roles::HasBuddyState), "hasBuddyState" },
        { static_cast<int>(Roles::Hops), "hops" },
        { static_cast<int>(Roles::CallType), "callType" },
        { static_cast<int>(Roles::ChatProviderId), "chatProviderId" },
        { static_cast<int>(Roles::ChatRoomId), "chatRoomId" },
        { static_cast<int>(Roles::IsOwnMessage), "isOwnMessage" },
    };
}

int ActivitiesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return m_entries.size();
}

QVariant ActivitiesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.size()) {
        return QVariant();
    }

    const auto &entry = m_entries.at(index.row());

    if (entry.kind != Kind::ChatMessage) {
        const auto item = entry.callItem;
        if (!item) {
            return QVariant();
        }

        ContactInfo contactInfo =
                PhoneNumberUtil::instance().contactInfoBySipUrl(item->remoteUrl());

        switch (role) {
        case static_cast<int>(Roles::Id):
            return item->dataBaseId();

        case static_cast<int>(Roles::Day):
            return entry.time.date();

        case static_cast<int>(Roles::Time):
            return entry.time;

        case static_cast<int>(Roles::Kind):
            return static_cast<int>(entry.kind);

        case static_cast<int>(Roles::IsSIPCall):
            return entry.kind == Kind::SIPCall;

        case static_cast<int>(Roles::IsJitsiMeetCall):
            return entry.kind == Kind::JitsiMeetCall;

        case static_cast<int>(Roles::IsChatMessage):
            return false;

        case static_cast<int>(Roles::Title):
            return !contactInfo.displayName.isEmpty() ? contactInfo.displayName
                                                      : contactInfo.phoneNumber;

        case static_cast<int>(Roles::Subtitle): {
            const auto c = contactInfo.contact;
            return c && !c->company().isEmpty() ? c->company() : "";
        }

        case static_cast<int>(Roles::Text):
            return contactInfo.phoneNumber;

        case static_cast<int>(Roles::Location): {
            QString str;

            if (!contactInfo.city.isEmpty()) {
                str = contactInfo.city;
            }

            if (!contactInfo.countries.isEmpty()) {
                if (!str.isEmpty()) {
                    str += ", ";
                }
                str += contactInfo.countries.join(", ");
            }
            return str;
        }

        case static_cast<int>(Roles::HasAvatar):
            return contactInfo.contact && contactInfo.contact->hasAvatar();

        case static_cast<int>(Roles::AvatarPath): {
            const auto c = contactInfo.contact;
            return c && c->hasAvatar() ? c->avatarPath() : "";
        }

        case static_cast<int>(Roles::Account):
            return item->account();

        case static_cast<int>(Roles::ContactId):
            return item->contactId();

        case static_cast<int>(Roles::RemoteUrl):
            return item->remoteUrl();

        case static_cast<int>(Roles::RemotePhoneNumber):
            return contactInfo.phoneNumber;

        case static_cast<int>(Roles::DurationSeconds):
            return item->durationSeconds();

        case static_cast<int>(Roles::WasEstablished):
            return item->durationSeconds() > 0;

        case static_cast<int>(Roles::IsAnonymous):
            return contactInfo.isAnonymous;

        case static_cast<int>(Roles::IsFavorite):
            return NumberStats::instance().isFavorite(contactInfo.phoneNumber);

        case static_cast<int>(Roles::IsBlocked): {
            const auto c = contactInfo.contact;
            const bool contactBlocked = c && SIPCallManager::instance().isContactBlocked(c->id());
            if (contactBlocked) {
                return true;
            }
            return SIPCallManager::instance().isPhoneNumberBlocked(contactInfo.phoneNumber);
        }

        case static_cast<int>(Roles::HasBuddyState):
            return item->isSipSubscriptable();

        case static_cast<int>(Roles::Hops): {
            const auto &hops = item->hops();

            if (hops.isEmpty()) {
                return hops;
            }

            QStringList l;
            l.reserve(hops.size());
            const auto &addressBook = AddressBook::instance();
            for (const auto &hop : hops) {
                const Contact *contact = addressBook.lookupByNumber(hop);
                if (contact && !contact->name().isEmpty()) {
                    l.append(QString("%1 (%2)").arg(contact->name(), hop));
                } else {
                    l.append(hop);
                }
            }

            return l;
        }

        case static_cast<int>(Roles::CallType):
            return static_cast<int>(item->type());

        case static_cast<int>(Roles::ChatProviderId):
        case static_cast<int>(Roles::ChatRoomId):
        case static_cast<int>(Roles::IsOwnMessage):
            return QVariant();

        default:
            return QVariant();
        }
    }

    switch (role) {
    case static_cast<int>(Roles::Id):
        return entry.chatProviderId + '|' + entry.chatRoomId + '|'
                + entry.time.toString(Qt::ISODateWithMs) + '|' + entry.senderName;

    case static_cast<int>(Roles::Day):
        return entry.time.date();

    case static_cast<int>(Roles::Time):
        return entry.time;

    case static_cast<int>(Roles::Kind):
        return static_cast<int>(Kind::ChatMessage);

    case static_cast<int>(Roles::IsChatMessage):
        return true;

    case static_cast<int>(Roles::IsSIPCall):
    case static_cast<int>(Roles::IsJitsiMeetCall):
        return false;

    case static_cast<int>(Roles::Title):
        return entry.roomName;

    case static_cast<int>(Roles::Subtitle):
        return entry.senderName;

    case static_cast<int>(Roles::Text):
        return entry.messageText;

    case static_cast<int>(Roles::Location):
        return "";

    case static_cast<int>(Roles::HasAvatar):
        return !entry.avatarPath.isEmpty();

    case static_cast<int>(Roles::AvatarPath):
        return entry.avatarPath;

    case static_cast<int>(Roles::ChatProviderId):
        return entry.chatProviderId;

    case static_cast<int>(Roles::ChatRoomId):
        return entry.chatRoomId;

    case static_cast<int>(Roles::IsOwnMessage):
        return entry.isOwnMessage;

    default:
        return QVariant();
    }
}

void ActivitiesModel::populateCallHistory()
{
    // The call history is already sorted by time descending, so the entries can be taken over
    // in order.
    const auto items = CallHistory::instance().historyItems();
    m_entries.reserve(items.size());
    for (const auto item : items) {
        Entry entry;
        entry.kind = item->type() & CallHistoryItem::Type::JitsiMeetCall ? Kind::JitsiMeetCall
                                                                         : Kind::SIPCall;
        entry.time = item->time();
        entry.callItem = item;
        m_entries.push_back(entry);
    }
}

void ActivitiesModel::subscribeToProviders()
{
    const auto providers = ChatConnectorManager::instance().chatConnectors();
    for (auto *provider : providers) {
        connect(provider, &IChatProvider::chatRoomAdded, this,
                [this](qsizetype, IChatRoom *room, const QString &) { subscribeToRoom(room); });

        const auto roomCount = provider->chatRoomsCount();
        for (qsizetype i = 0; i < roomCount; ++i) {
            subscribeToRoom(provider->chatRoomByIndex(i));
        }
    }
}

void ActivitiesModel::subscribeToRoom(IChatRoom *room)
{
    if (!room || m_subscribedRooms.contains(room)) {
        return;
    }
    m_subscribedRooms.insert(room);

    connect(room, &QObject::destroyed, this, [this, room]() { m_subscribedRooms.remove(room); });
    connect(room, &IChatRoom::chatMessageAdded, this,
            [this](qsizetype, ChatMessage *message) { handleChatMessageAdded(message); });
    connect(room, &IChatRoom::chatMessageOutOfSequenceReceived, this,
            [this](ChatMessage *message) { handleChatMessageAdded(message); });
}

void ActivitiesModel::handleCallItemAdded(qsizetype index, CallHistoryItem *item)
{
    Q_UNUSED(index)

    Entry entry;
    entry.kind = item->type() & CallHistoryItem::Type::JitsiMeetCall ? Kind::JitsiMeetCall
                                                                     : Kind::SIPCall;
    entry.time = item->time();
    entry.callItem = item;
    insertEntry(entry);
}

void ActivitiesModel::handleCallItemChanged(qsizetype index, CallHistoryItem *item)
{
    Q_UNUSED(index)

    for (qsizetype i = 0; i < m_entries.size(); ++i) {
        if (m_entries.at(i).callItem == item) {
            Q_EMIT dataChanged(createIndex(i, 0), createIndex(i, 0));
            return;
        }
    }
}

void ActivitiesModel::handleChatMessageAdded(ChatMessage *message)
{
    if (!message || message->isStateUpdate()
        || static_cast<bool>(message->flags() & ChatMessage::Flag::SystemMessage)) {
        return;
    }

    // Only messages that arrive while GOnnect is running shall be recorded. Messages that have
    // been fetched into a room on demand (i.e. its older history) must not end up here.
    if (const auto room = message->chatRoom()) {
        if (room->isLoadingMessageHistory()) {
            return;
        }
    }

    Entry entry;
    entry.kind = Kind::ChatMessage;
    entry.time = message->timestamp();
    entry.isOwnMessage = static_cast<bool>(message->flags() & ChatMessage::Flag::OwnMessage);
    entry.senderName = message->nickName();
    entry.messageText = chatMessagePreview(message);

    if (const auto room = message->chatRoom()) {
        entry.chatRoomId = room->id();
        entry.roomName = room->name();

        if (const auto provider = qobject_cast<IChatProvider *>(room->parent())) {
            entry.chatProviderId = provider->id();
        }

        if (const auto user = room->chatUserById(message->fromId())) {
            // Chat user avatar paths are full file URLs (see makeDataRootPath), while the entries
            // of call kinds hold plain filesystem paths, so the file scheme is stripped here.
            QString avatarPath = user->avatarPath();
            if (avatarPath.startsWith("file://")) {
                avatarPath = avatarPath.mid(7);
            }
            entry.avatarPath = avatarPath;
        }
    }

    insertEntry(entry);
}

void ActivitiesModel::insertEntry(Entry entry)
{
    // The entries are kept sorted by time descending, so the new entry is inserted before the
    // first entry that is not newer.
    qsizetype insertIndex = m_entries.size();
    for (qsizetype i = 0; i < m_entries.size(); ++i) {
        if (m_entries.at(i).time <= entry.time) {
            insertIndex = i;
            break;
        }
    }

    beginInsertRows(QModelIndex(), insertIndex, insertIndex);
    m_entries.insert(insertIndex, entry);
    endInsertRows();

    trimToLimit();
}

void ActivitiesModel::trimToLimit()
{
    // Drop the oldest entries if the limit has been reached.
    while (m_limit >= 0 && m_entries.size() > m_limit) {
        const auto lastIndex = m_entries.size() - 1;
        beginRemoveRows(QModelIndex(), lastIndex, lastIndex);
        m_entries.removeAt(lastIndex);
        endRemoveRows();
    }
}

void ActivitiesModel::refreshCallRoles(const QList<int> &roles)
{
    for (qsizetype i = 0; i < m_entries.size(); ++i) {
        if (m_entries.at(i).kind != Kind::ChatMessage) {
            Q_EMIT dataChanged(createIndex(i, 0), createIndex(i, 0), roles);
        }
    }
}