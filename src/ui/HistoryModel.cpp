#include "HistoryModel.h"
#include "CallHistory.h"
#include "CallHistoryItem.h"
#include "PhoneNumberUtil.h"
#include "AvatarManager.h"
#include "NumberStats.h"
#include "SIPCallManager.h"
#include "AddressBook.h"

HistoryModel::HistoryModel(QObject *parent) : QAbstractListModel{ parent }
{
    const auto &history = CallHistory::instance();
    connect(&history, &CallHistory::itemAdded, this, &HistoryModel::resetModel);

    connect(&history, &CallHistory::dataChanged, this, [this](qsizetype index, CallHistoryItem *) {
        auto idx = createIndex(index, 0);
        emit dataChanged(idx, idx);
    });

    connect(&AvatarManager::instance(), &AvatarManager::avatarsLoaded, this, [this]() {
        const auto startIndex = createIndex(0, 0);
        const auto endIndex = createIndex(rowCount(QModelIndex()), 0);
        emit dataChanged(
                startIndex, endIndex,
                { static_cast<int>(Roles::HasAvatar), static_cast<int>(Roles::AvatarPath) });
    });
    connect(&AvatarManager::instance(), &AvatarManager::avatarAdded, this, [this](QString) {
        const auto startIndex = createIndex(0, 0);
        const auto endIndex = createIndex(rowCount(QModelIndex()), 0);
        emit dataChanged(
                startIndex, endIndex,
                { static_cast<int>(Roles::HasAvatar), static_cast<int>(Roles::AvatarPath) });
    });

    connect(&SIPCallManager::instance(), &SIPCallManager::blocksChanged, this,
            &HistoryModel::resetModel);

    const auto &numStats = NumberStats::instance();
    connect(&numStats, &NumberStats::favoriteAdded, this, &HistoryModel::handleFavoriteToggle);
    connect(&numStats, &NumberStats::favoriteRemoved, this, &HistoryModel::handleFavoriteToggle);
    connect(&numStats, &NumberStats::modelReset, this, &HistoryModel::resetModel);

    connect(&AddressBook::instance(), &AddressBook::contactsReady, this, &HistoryModel::resetModel);

    connect(this, &HistoryModel::limitChanged, this, &HistoryModel::resetModel);
}

QHash<int, QByteArray> HistoryModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::Id), "id" },
        { static_cast<int>(Roles::ContactId), "contactId" },
        { static_cast<int>(Roles::Day), "day" },
        { static_cast<int>(Roles::Time), "time" },
        { static_cast<int>(Roles::Account), "account" },
        { static_cast<int>(Roles::ContactName), "contactName" },
        { static_cast<int>(Roles::Company), "company" },
        { static_cast<int>(Roles::Location), "location" },
        { static_cast<int>(Roles::HasAvatar), "hasAvatar" },
        { static_cast<int>(Roles::AvatarPath), "avatarPath" },
        { static_cast<int>(Roles::RemoteUrl), "remoteUrl" },
        { static_cast<int>(Roles::RemotePhoneNumber), "remotePhoneNumber" },
        { static_cast<int>(Roles::DurationSeconds), "durationSeconds" },
        { static_cast<int>(Roles::WasEstablished), "wasEstablished" },
        { static_cast<int>(Roles::IsAnonymous), "isAnonymous" },
        { static_cast<int>(Roles::IsFavorite), "isFavorite" },
        { static_cast<int>(Roles::IsBlocked), "isBlocked" },
        { static_cast<int>(Roles::Type), "type" },
        { static_cast<int>(Roles::HasBuddyState), "hasBuddyState" },
    };
}

void HistoryModel::handleFavoriteToggle(const NumberStat *item)
{
    const auto phoneNumber = item->phoneNumber;
    const auto items = CallHistory::instance().historyItems();

    for (int i = 0; i < items.length(); ++i) {
        const QString &number = PhoneNumberUtil::instance()
                                        .contactInfoBySipUrl(items.at(i)->remoteUrl())
                                        .phoneNumber;
        if (number == phoneNumber) {
            auto idx = createIndex(i, 0);
            emit dataChanged(idx, idx, { static_cast<int>(Roles::IsFavorite) });
        }
    }
}

void HistoryModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

int HistoryModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    int size = CallHistory::instance().historyItems().size();
    if (m_limit >= 0) {
        size = std::min(m_limit, size);
    }
    return size;
}

QVariant HistoryModel::data(const QModelIndex &index, int role) const
{
    const auto items = CallHistory::instance().historyItems();
    const auto item = items.at(index.row());

    ContactInfo contactInfo = PhoneNumberUtil::instance().contactInfoBySipUrl(item->remoteUrl());

    switch (role) {
    case static_cast<int>(Roles::Id):
        return item->dataBaseId();

    case static_cast<int>(Roles::ContactId):
        return item->contactId();

    case static_cast<int>(Roles::Day):
        return item->time().date();

    case static_cast<int>(Roles::Time):
        return item->time();

    case static_cast<int>(Roles::Account):
        return item->account();

    case static_cast<int>(Roles::ContactName): {
        return !contactInfo.displayName.isEmpty() ? contactInfo.displayName : "";
    }

    case static_cast<int>(Roles::Company): {
        auto c = contactInfo.contact;
        return c && !c->company().isEmpty() ? c->company() : "";
    }

    case static_cast<int>(Roles::Location): {
        QString str;

        if (!contactInfo.city.isEmpty()) {
            str = contactInfo.city;
        }

        if (!contactInfo.countries.isEmpty()) {
            if (!str.isEmpty()) {
                str += ", ";
            }
            str = contactInfo.countries.join(", ");
        }
        return str;
    }

    case static_cast<int>(Roles::HasAvatar):
        return contactInfo.contact && contactInfo.contact->hasAvatar();

    case static_cast<int>(Roles::AvatarPath): {
        const auto c = contactInfo.contact;
        return c && c->hasAvatar() ? c->avatarPath() : "";
    }

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

    case static_cast<int>(Roles::Type):
        return static_cast<int>(item->type());

    case static_cast<int>(Roles::HasBuddyState):
        return item->isSipSubscriptable();

    default:
        return QVariant();
    }
}
