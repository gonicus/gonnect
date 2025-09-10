#include "SearchListModel.h"
#include "AddressBook.h"
#include "Contact.h"
#include "NumberStats.h"
#include "NumberStat.h"

#include <QRegularExpression>

SearchListModel::SearchListModel(QObject *parent) : QAbstractListModel{ parent }
{
    connect(this, &SearchListModel::searchPhraseChanged, this,
            &SearchListModel::updateSearchResults);
    connect(this, &QAbstractListModel::modelReset, this, &SearchListModel::numbersIndexUpdated);

    const auto &numStat = NumberStats::instance();
    connect(&numStat, &NumberStats::favoriteAdded, this, &SearchListModel::handleFavoriteToggle);
    connect(&numStat, &NumberStats::favoriteRemoved, this, &SearchListModel::handleFavoriteToggle);
}

void SearchListModel::updateSearchResults()
{
    QList<Contact *> results;
    const auto searchPhrase = m_searchPhrase.trimmed();

    if (searchPhrase.size() >= 3) {
        results = AddressBook::instance().search(searchPhrase);
    }

    if (results != m_model) {
        beginResetModel();
        m_model = results;

        // Build numbers count list
        m_totalNumbersCount = 0;
        m_numberIndexOffsets.clear();
        m_numberIndexOffsets.reserve(m_model.size());

        for (const auto contact : std::as_const(m_model)) {
            m_numberIndexOffsets.append(m_totalNumbersCount);
            m_totalNumbersCount += contact->phoneNumbers().size();
        }

        endResetModel();
    }
}

void SearchListModel::handleFavoriteToggle(const NumberStat *numberStat)
{
    const auto &phoneNumber = numberStat->phoneNumber;

    for (int i = 0; i < m_model.size(); ++i) {
        const auto numbers = m_model.at(i)->phoneNumbers();
        for (const auto &numObj : numbers) {
            if (numObj.number == phoneNumber) {
                const auto idx = createIndex(i, 0);
                Q_EMIT dataChanged(idx, idx, { static_cast<int>(Roles::Numbers) });
                return;
            }
        }
    }
}

QHash<int, QByteArray> SearchListModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::Id), "id" },
        { static_cast<int>(Roles::Name), "name" },
        { static_cast<int>(Roles::Company), "company" },
        { static_cast<int>(Roles::HasAvatar), "hasAvatar" },
        { static_cast<int>(Roles::AvatarPath), "avatarPath" },
        { static_cast<int>(Roles::SubscriptableNumber), "subscriptableNumber" },
        { static_cast<int>(Roles::Numbers), "numbers" },
        { static_cast<int>(Roles::NumbersCount), "numbersCount" },
        { static_cast<int>(Roles::NumbersIndexOffset), "numbersIndexOffset" },
        { static_cast<int>(Roles::SourcePriority), "sourcePriority" },
        { static_cast<int>(Roles::SourceDisplayName), "sourceDisplayName" },
    };
}

QString SearchListModel::phoneNumberByIndex(quint16 index) const
{
    const auto l = m_numberIndexOffsets.size();
    for (int i = 0; i < l; ++i) {
        if (m_numberIndexOffsets.at(i) <= index
            && (i == l - 1 || m_numberIndexOffsets.at(i + 1) > index)) {
            const auto offset = m_numberIndexOffsets.at(i);
            const auto innerIndex = index - offset;
            const auto contact = m_model.at(i);
            if (contact && 0 <= innerIndex && innerIndex < contact->phoneNumbers().size()) {
                return contact->phoneNumbers().at(innerIndex).number;
            } else {
                return ""; // Invalid index
            }
        }
    }

    return "";
}

QString SearchListModel::contactIdByIndex(quint16 index) const
{
    const auto l = m_numberIndexOffsets.size();
    for (int i = 0; i < l; ++i) {
        if (m_numberIndexOffsets.at(i) <= index
            && (i == l - 1 || m_numberIndexOffsets.at(i + 1) > index)) {
            const auto contact = m_model.at(i);
            return contact->id();
        }
    }

    return "";
}

int SearchListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_model.size();
}

QVariant SearchListModel::data(const QModelIndex &index, int role) const
{
    const auto row = index.row();
    const Contact *contact = m_model.at(row);

    switch (role) {

    case static_cast<int>(Roles::Id):
        return contact->id();

    case static_cast<int>(Roles::Company):
        return contact->company();

    case static_cast<int>(Roles::HasAvatar):
        return contact->hasAvatar();

    case static_cast<int>(Roles::AvatarPath):
        return contact->avatarPath();

    case static_cast<int>(Roles::SubscriptableNumber):
        return contact->subscriptableNumber();

    case static_cast<int>(Roles::NumbersCount):
        return contact->phoneNumbers().size();

    case static_cast<int>(Roles::Numbers): {
        QVariantList result;
        const auto numbers = contact->phoneNumbers();
        for (const auto &item : numbers) {
            QVariantMap map;
            map.insert("type", static_cast<int>(item.type));
            map.insert("number", item.number);
            map.insert("isSipStatusSubscriptable", item.isSipSubscriptable);
            map.insert("isFavorite", NumberStats::instance().isFavorite(item.number));
            result.append(map);
        }

        return result;
    }

    case static_cast<int>(Roles::NumbersIndexOffset):
        return m_numberIndexOffsets.at(row);

    case static_cast<int>(Roles::SourcePriority):
        return contact->contactSourceInfo().prio;

    case static_cast<int>(Roles::SourceDisplayName):
        return contact->contactSourceInfo().displayName;

    default:
        return contact->name();
    }
}
