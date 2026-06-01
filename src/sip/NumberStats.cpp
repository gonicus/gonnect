#include "NumberStats.h"
#include "CallHistory.h"
#include "AddressBook.h"
#include "NumberStat.h"
#include "PhoneNumberCallCount.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QLoggingCategory>
#include <utility>

Q_LOGGING_CATEGORY(lcNumberStats, "gonnect.app.NumberStats")

using namespace std::chrono_literals;

NumberStats::NumberStats(QObject *parent) : QObject{ parent }
{
    CallHistory::instance(); // Ensure database is properly set up which happens in CallHistory ctor

    m_debounceAddressBookUpdateTimer.setSingleShot(true);
    m_debounceAddressBookUpdateTimer.setInterval(5ms);
    m_debounceAddressBookUpdateTimer.callOnTimeout(this, [this]() {
        initialRead();
        readNumberOfCalls();
    });

    connect(&AddressBook::instance(), &AddressBook::contactsReady, this,
            [this]() { m_debounceAddressBookUpdateTimer.start(); });
    connect(&AddressBook::instance(), &AddressBook::contactAdded, this,
            [this]() { m_debounceAddressBookUpdateTimer.start(); });
    connect(&AddressBook::instance(), &AddressBook::contactModified, this,
            [this]() { m_debounceAddressBookUpdateTimer.start(); });

    initialRead();
    readNumberOfCalls();
}

void NumberStats::initialRead()
{
    m_statItems.clear();
    m_favoriteLookup.clear();
    m_statItemsLookup.clear();

    auto db = QSqlDatabase::database();

    if (!db.open()) {
        qCCritical(lcNumberStats) << "Unable to open database:" << db.lastError().text();
    } else {
        qCInfo(lcNumberStats) << "Successfully opened database";

        // Read history
        qCInfo(lcNumberStats) << "Collecting phone number flags and stats from database";

        QSqlQuery query(db);
        query.prepare("SELECT * FROM contactflags;");

        if (!query.exec()) {
            qCCritical(lcNumberStats)
                    << "Error on executing SQL query:" << query.lastError().text();
        } else {
            while (query.next()) {
                auto item = new NumberStat;

                item->phoneNumber = query.value("phoneNumber").toString();
                item->isBlocked = query.value("isBlocked").toBool();
                item->isFavorite = query.value("isFavorite").toBool();
                item->contactType =
                        static_cast<NumberStats::ContactType>(query.value("type").toUInt());
                item->contact = AddressBook::instance().lookupByNumber(item->phoneNumber);

                m_statItemsLookup.insert(item->phoneNumber, item);
                m_statItems.append(item);
                if (item->isFavorite) {
                    m_favoriteLookup.insert(item->phoneNumber, item);
                }

                Q_EMIT numberStatAdded(m_statItems.size() - 1);
            }

            qCInfo(lcNumberStats) << "Read" << m_statItems.size() << "flags/stats from database";
        }
    }

    Q_EMIT modelReset();
}

void NumberStats::readNumberOfCalls()
{
    auto db = QSqlDatabase::database();

    if (!db.open()) {
        qCCritical(lcNumberStats) << "Unable to open call history database:"
                                  << db.lastError().text();
    } else {
        qCInfo(lcNumberStats) << "Successfully opened history database";

        const auto sipCallValue = std::to_underlying(CallHistoryItem::Type::SIPCall);
        QSqlQuery query(db);
        query.prepare("SELECT remoteUrl, COUNT(*) as numberOfCalls FROM HISTORY WHERE (type & "
                      ":typeValue) != 0 GROUP BY remoteUrl, account ORDER BY numberOfCalls DESC;");
        query.bindValue(":typeValue", sipCallValue);

        if (!query.exec()) {
            qCCritical(lcNumberStats)
                    << "Error on executing SQL query:" << query.lastError().text();
        } else {
            while (query.next()) {
                const auto phoneNumber = PhoneNumberUtil::cleanPhoneNumber(
                        PhoneNumberUtil::numberFromSipUrl(query.value("remoteUrl").toString()));

                if (!phoneNumber.isEmpty()) {
                    const auto count = query.value("numberOfCalls").toUInt();

                    // Because remoteUrls are not
                    if (auto *item = m_callCountLookup.value(phoneNumber, nullptr)) {
                        item->count += count;
                    } else {
                        createAndAddCountObject(phoneNumber, count);
                    }
                }
            }
        }
    }
}

NumberStats::~NumberStats()
{
    m_statItemsLookup.clear();
    m_favoriteLookup.clear();
    qDeleteAll(m_statItems);
    m_statItems.clear();
}

void NumberStats::incrementCallCount(const QString &phoneNumber)
{
    auto db = QSqlDatabase::database();

    if (!db.open()) {
        qCCritical(lcNumberStats) << "Unable to open call history database:"
                                  << db.lastError().text();
    } else {
        qCInfo(lcNumberStats) << "Successfully opened history database";

        if (ensureFlaggedNumberExists(phoneNumber)) {
            auto *countObj = m_callCountLookup.value(phoneNumber, nullptr);
            if (countObj) {
                countObj->count++;

                std::ranges::sort(m_callCounts,
                                  [](const PhoneNumberCallCount *left,
                                     const PhoneNumberCallCount *right) -> bool {
                                      return left->count > right->count;
                                  });
            } else {
                countObj = createAndAddCountObject(phoneNumber, 1);
            }

            Q_EMIT countChanged(m_callCounts.indexOf(countObj));
        }
    }
}

QList<NumberStat *> NumberStats::favorites() const
{
    QList<NumberStat *> result;

    for (auto item : m_statItems) {
        if (item->isFavorite) {
            result.append(item);
        }
    }

    std::sort(result.begin(), result.end(),
              [](const NumberStat *left, const NumberStat *right) -> bool {
                  if (left->contact && right->contact) {
                      return left->contact->name() < right->contact->name();
                  }
                  if (left->contact && !right->contact) {
                      return false;
                  }
                  if (!left->contact && right->contact) {
                      return true;
                  }
                  return left->phoneNumber < right->phoneNumber;
              });

    return result;
}

QStringList NumberStats::mostCalled(quint8 limit, bool includeFavorites) const
{
    QStringList result;
    result.reserve(limit);

    if (includeFavorites) {
        const auto l = std::min(static_cast<qsizetype>(limit), m_callCounts.size());
        for (qsizetype i = 0; i < l; ++i) {
            result.append(m_callCounts.at(i)->phoneNumber);
        }
    } else {
        for (const auto *countObj : std::as_const(m_callCounts)) {
            if (!m_favoriteLookup.contains(countObj->phoneNumber)) {
                result.append(countObj->phoneNumber);
            }

            if (result.size() == limit) {
                break;
            }
        }
    }

    return result;
}

bool NumberStats::isFavorite(const QString &phoneNumber) const
{
    return m_favoriteLookup.contains(phoneNumber);
}

void NumberStats::toggleFavorite(const QString &phoneNumber,
                                 const NumberStats::ContactType contactType)
{
    ensureFlaggedNumberExists(phoneNumber, contactType);

    auto item = m_statItemsLookup.value(phoneNumber);
    if (item->isFavorite) {
        item->isFavorite = false;
        m_favoriteLookup.remove(phoneNumber);
        Q_EMIT favoriteRemoved(item);
    } else {
        item->isFavorite = true;
        m_favoriteLookup.insert(phoneNumber, item);
        Q_EMIT favoriteAdded(item);
    }

    auto db = QSqlDatabase::database();

    if (!db.open()) {
        qCCritical(lcNumberStats) << "Unable to open call history database:"
                                  << db.lastError().text();
    } else {
        qCInfo(lcNumberStats) << "Successfully opened history database";

        QSqlQuery query(db);

        // Update DB entry
        qCInfo(lcNumberStats) << "Updating favorite flag for number" << phoneNumber
                              << "in database";
        query.prepare(
                "UPDATE contactflags SET isFavorite = :favValue WHERE phoneNumber = :phoneNumber;");
        query.bindValue(":favValue", item->isFavorite ? 1 : 0);
        query.bindValue(":phoneNumber", phoneNumber);

        if (!query.exec()) {
            qCCritical(lcNumberStats)
                    << "Error on executing SQL query:" << query.lastError().text();
        }
    }
}

const NumberStat *NumberStats::numberStat(const QString &phoneNumber) const
{
    return m_statItemsLookup.value(phoneNumber, nullptr);
}

bool NumberStats::ensureFlaggedNumberExists(const QString &phoneNumber,
                                            const NumberStats::ContactType contactType)
{
    if (m_statItemsLookup.contains(phoneNumber)) {
        return true;
    }

    auto db = QSqlDatabase::database();

    if (!db.open()) {
        qCCritical(lcNumberStats) << "Unable to open call history database:"
                                  << db.lastError().text();
        return false;

    } else {
        qCInfo(lcNumberStats) << "Successfully opened history database";

        QSqlQuery query(db);
        query.prepare("INSERT INTO contactflags (phonenumber, isFavorite, isBlocked, type) "
                      "VALUES (:phoneNumber, 0, 0, :type);");
        query.bindValue(":phoneNumber", phoneNumber);
        query.bindValue(":type", std::to_underlying(contactType));

        if (!query.exec()) {
            qCCritical(lcNumberStats)
                    << "Error on executing SQL query:" << query.lastError().text();
            return false;
        }

        auto statItem = new NumberStat;
        statItem->phoneNumber = phoneNumber;
        statItem->contactType = contactType;
        statItem->contact = AddressBook::instance().lookupByNumber(phoneNumber);
        m_statItems.append(statItem);
        m_statItemsLookup.insert(phoneNumber, statItem);
        Q_EMIT numberStatAdded(m_statItems.size() - 1);
        return true;
    }

    return false;
}

PhoneNumberCallCount *NumberStats::createAndAddCountObject(const QString &phoneNumber,
                                                           quint32 count)
{
    auto *countObj = new PhoneNumberCallCount(this);
    countObj->phoneNumber = phoneNumber;
    countObj->count = count;

    m_callCounts.append(countObj);
    m_callCountLookup.insert(phoneNumber, countObj);

    return countObj;
}

QDebug operator<<(QDebug debug, const NumberStats &stats)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "NumberStats(" << stats.statsAndFlags().size() << " items)";
    return debug;
}

QDebug operator<<(QDebug debug, const NumberStat &statItem)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "NumberStat(" << statItem.phoneNumber
                    << ", is favorite: " << statItem.isFavorite
                    << ", is blocked: " << statItem.isBlocked << ")";
    return debug;
}
