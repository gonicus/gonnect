#include "HistoryContactSearchModel.h"
#include "CallHistory.h"
#include "FuzzyCompare.h"
#include "NumberStats.h"
#include "NumberStat.h"
#include "SIPCallManager.h"

HistoryContactSearchModel::HistoryContactSearchModel(QObject *parent) : QAbstractListModel{ parent }
{
    connect(this, &HistoryContactSearchModel::searchTextChanged, this,
            &HistoryContactSearchModel::updateModel);

    connect(&NumberStats::instance(), &NumberStats::favoriteAdded, this,
            &HistoryContactSearchModel::onFavoriteChanged);
    connect(&NumberStats::instance(), &NumberStats::favoriteRemoved, this,
            &HistoryContactSearchModel::onFavoriteChanged);

    connect(&SIPCallManager::instance(), &SIPCallManager::blocksChanged, this, [this]() {
        const auto count = rowCount(QModelIndex());
        if (count) {
            Q_EMIT dataChanged(createIndex(0, 0), createIndex(count - 1, 0),
                               { static_cast<int>(Roles::IsBlocked) });
        }
    });
}

void HistoryContactSearchModel::updateModel()
{
    beginResetModel();
    m_resultList.clear();

    if (m_searchText.length() >= 3) {
        const auto items = CallHistory::instance().historyItems();

        // Build model
        for (const auto item : items) {
            const ContactInfo contactInfo =
                    PhoneNumberUtil::instance().contactInfoBySipUrl(item->remoteUrl());

            if (item->remoteUrl().contains(m_searchText, Qt::CaseInsensitive)
                || contactInfo.displayName.contains(m_searchText, Qt::CaseInsensitive)) {

                const auto &remoteUrl = item->remoteUrl();
                const bool isPhoneNumber = PhoneNumberUtil::isSipUri(remoteUrl);
                const qreal dist = std::max(
                        FuzzyCompare::jaroWinklerDistance(remoteUrl, m_searchText),
                        FuzzyCompare::jaroWinklerDistance(contactInfo.displayName, m_searchText));

                Item newItem = { isPhoneNumber,
                                 isPhoneNumber ? PhoneNumberUtil::numberFromSipUrl(remoteUrl)
                                               : remoteUrl,
                                 contactInfo.displayName, dist, item->time() };

                bool isAlreadyThere = false;
                for (auto &existingItem : m_resultList) {
                    if (existingItem == newItem) {
                        existingItem.lastCall = std::max(existingItem.lastCall, item->time());
                        isAlreadyThere = true;
                        break;
                    }
                }

                if (!isAlreadyThere) {
                    m_resultList.append(newItem);
                }
            }
        }

        // Sorting
        std::sort(m_resultList.begin(), m_resultList.end(),
                  [](const Item left, const Item right) -> bool {
                      if (left.searchStringDistance == right.searchStringDistance) {
                          return left.lastCall > right.lastCall;
                      }
                      return left.searchStringDistance > right.searchStringDistance;
                  });
    }
    endResetModel();
}

void HistoryContactSearchModel::onFavoriteChanged(const NumberStat *numStatItem)
{
    for (qsizetype i = 0; i < m_resultList.size(); ++i) {
        if (std::as_const(m_resultList).at(i).url == numStatItem->phoneNumber) {
            const auto index = createIndex(i, 0);
            Q_EMIT dataChanged(index, index, { static_cast<int>(Roles::IsFavorite) });
            return;
        }
    }
}

QHash<int, QByteArray> HistoryContactSearchModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::DisplayName), "displayName" },
        { static_cast<int>(Roles::Url), "url" },
        { static_cast<int>(Roles::IsPhoneNumber), "isPhoneNumber" },
        { static_cast<int>(Roles::IsFavorite), "isFavorite" },
        { static_cast<int>(Roles::IsBlocked), "isBlocked" },
        { static_cast<int>(Roles::IsAnonymous), "isAnonymous" },
        { static_cast<int>(Roles::IsSipSubscriptable), "isSipSubscriptable" },
        { static_cast<int>(Roles::LastCall), "lastCall" },
        { static_cast<int>(Roles::SearchStringDistance), "searchStringDistance" },
    };
}

int HistoryContactSearchModel::rowCount(const QModelIndex &) const
{
    return m_resultList.size();
}

QVariant HistoryContactSearchModel::data(const QModelIndex &index, int role) const
{
    const auto &item = m_resultList.at(index.row());

    switch (role) {
    case static_cast<int>(Roles::Url):
        return item.url;

    case static_cast<int>(Roles::IsPhoneNumber):
        return item.isPhoneNumber;

    case static_cast<int>(Roles::IsFavorite): {
        const ContactInfo contactInfo = PhoneNumberUtil::instance().contactInfoBySipUrl(item.url);
        return NumberStats::instance().isFavorite(contactInfo.phoneNumber);
    }

    case static_cast<int>(Roles::IsBlocked): {
        const ContactInfo contactInfo = PhoneNumberUtil::instance().contactInfoBySipUrl(item.url);
        const Contact *contact = contactInfo.contact;

        const bool contactBlocked =
                contact && SIPCallManager::instance().isContactBlocked(contact->id());
        if (contactBlocked) {
            return true;
        }
        return SIPCallManager::instance().isPhoneNumberBlocked(contactInfo.phoneNumber);
    }

    case static_cast<int>(Roles::IsAnonymous):
        return PhoneNumberUtil::isNumberAnonymous(item.url);

    case static_cast<int>(Roles::IsSipSubscriptable):
        return PhoneNumberUtil::instance().contactInfoBySipUrl(item.url).isSipSubscriptable;

    case static_cast<int>(Roles::LastCall):
        return item.lastCall;

    case static_cast<int>(Roles::SearchStringDistance):
        return item.searchStringDistance;

    case static_cast<int>(Roles::DisplayName):
    default:
        return item.displayName;
    }
}

bool HistoryContactSearchModel::Item::operator!=(const Item &other) const
{
    return url != other.url || displayName != other.displayName;
}

bool HistoryContactSearchModel::Item::operator==(const Item &other) const
{
    return !(*this != other);
}
