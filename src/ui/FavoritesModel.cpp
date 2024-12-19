#include "FavoritesModel.h"
#include "NumberStats.h"
#include "Contact.h"
#include "PhoneNumberUtil.h"
#include "SIPCallManager.h"

FavoritesModel::FavoritesModel(QObject *parent) : QAbstractListModel{ parent }
{
    auto &numStats = NumberStats::instance();
    connect(&numStats, &NumberStats::favoriteAdded, this, &FavoritesModel::updateModel);
    connect(&numStats, &NumberStats::favoriteRemoved, this, &FavoritesModel::updateModel);
    connect(&numStats, &NumberStats::modelReset, this, &FavoritesModel::updateModel);

    connect(&SIPCallManager::instance(), &SIPCallManager::blocksChanged, this,
            &FavoritesModel::updateModel);

    updateModel();
}

QHash<int, QByteArray> FavoritesModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::PhoneNumber), "phoneNumber" },
        { static_cast<int>(Roles::ContactId), "contactId" },
        { static_cast<int>(Roles::Name), "name" },
        { static_cast<int>(Roles::HasBuddyState), "hasBuddyState" },
        { static_cast<int>(Roles::HasAvatar), "hasAvatar" },
        { static_cast<int>(Roles::AvatarPath), "avatarPath" },
        { static_cast<int>(Roles::IsAnonymous), "isAnonymous" },
        { static_cast<int>(Roles::IsBlocked), "isBlocked" },
        { static_cast<int>(Roles::NumberType), "numberType" },
    };
}

void FavoritesModel::updateModel()
{
    beginResetModel();
    m_favorites = NumberStats::instance().favorites();
    endResetModel();
}

int FavoritesModel::rowCount(const QModelIndex &) const
{
    return m_favorites.size();
}

QVariant FavoritesModel::data(const QModelIndex &index, int role) const
{
    const auto item = m_favorites.at(index.row());
    ContactInfo contactInfo = PhoneNumberUtil::instance().contactInfoBySipUrl(item->phoneNumber);

    switch (role) {
    case static_cast<int>(Roles::PhoneNumber):
        return item->phoneNumber;

    case static_cast<int>(Roles::ContactId):
        return item->contact ? item->contact->id() : "";

    case static_cast<int>(Roles::HasBuddyState):
        return item->contact
                ? item->contact->phoneNumberObject(item->phoneNumber).isSipSubscriptable
                : false;

    case static_cast<int>(Roles::NumberType):
        return static_cast<int>(item->contact
                                        ? item->contact->phoneNumberObject(item->phoneNumber).type
                                        : Contact::NumberType::Unknown);

    case static_cast<int>(Roles::HasAvatar):
        return item->contact && item->contact->hasAvatar();

    case static_cast<int>(Roles::AvatarPath):
        return (item->contact && item->contact->hasAvatar()) ? item->contact->avatarPath() : "";

    case static_cast<int>(Roles::IsAnonymous):
        return contactInfo.isAnonymous;

    case static_cast<int>(Roles::IsBlocked): {
        const bool isContactBlocked =
                item->contact && SIPCallManager::instance().isContactBlocked(item->contact->id());
        if (isContactBlocked) {
            return true;
        }
        return SIPCallManager::instance().isPhoneNumberBlocked(item->phoneNumber);
    }

    case static_cast<int>(Roles::Name):
    default:
        return item->contact ? item->contact->name() : "";
    }
}
