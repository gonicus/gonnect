#include "NumberStats.h"
#include "CallHistory.h"
#include "AddressBook.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcNumberStats, "gonnect.app.NumberSTats")

using namespace std::chrono_literals;

NumberStats::NumberStats(QObject *parent) : QObject{ parent }
{
    CallHistory::instance(); // Ensure database is properly set up which happens in CallHistory ctor

    m_debounceAddressBookUpdateTimer.setSingleShot(true);
    m_debounceAddressBookUpdateTimer.setInterval(5ms);
    m_debounceAddressBookUpdateTimer.callOnTimeout(this, &NumberStats::initialRead);

    connect(&AddressBook::instance(), &AddressBook::contactsReady, this,
            [this]() { m_debounceAddressBookUpdateTimer.start(); });
    connect(&AddressBook::instance(), &AddressBook::contactAdded, this,
            [this]() { m_debounceAddressBookUpdateTimer.start(); });

    initialRead();
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
                item->callCount = query.value("callcount").toUInt();
                item->isBlocked = query.value("isBlocked").toBool();
                item->isFavorite = query.value("isFavorite").toBool();
                item->contact = AddressBook::instance().lookupByNumber(item->phoneNumber);

                m_statItemsLookup.insert(item->phoneNumber, item);
                m_statItems.append(item);
                if (item->isFavorite) {
                    m_favoriteLookup.insert(item->phoneNumber, item);
                }

                emit numberStatAdded(m_statItems.size() - 1);
            }

            qCInfo(lcNumberStats) << "Read" << m_statItems.size() << "flags/stats from database";
        }
    }

    emit modelReset();
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

        QSqlQuery query(db);

        if (ensureFlaggedNumberExists(phoneNumber)) {
            auto statItem = m_statItemsLookup.value(phoneNumber);
            statItem->callCount++;
            emit countChanged(m_statItems.indexOf(statItem));

            // Update DB entry
            qCInfo(lcNumberStats) << "Updating call count for number" << phoneNumber
                                  << "in database";
            query.prepare("UPDATE contactflags SET callcount = callcount + 1 WHERE phoneNumber = "
                          ":phoneNumber;");
            query.bindValue(":phoneNumber", phoneNumber);

            if (!query.exec()) {
                qCCritical(lcNumberStats)
                        << "Error on executing SQL query:" << query.lastError().text();
            }
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
    auto copy = m_statItems;

    std::sort(copy.begin(), copy.end(),
              [](const NumberStat *left, const NumberStat *right) -> bool {
                  return left->callCount > right->callCount;
              });

    QStringList result;
    result.reserve(limit);
    quint8 count = 0;

    for (qsizetype i = 0, s = copy.size(); count < limit && i < s; ++i) {
        const auto item = copy.at(i);
        if (includeFavorites || !item->isFavorite) {
            result.append(item->phoneNumber);
            ++count;
        }
    }
    return result;
}

void NumberStats::toggleFavorite(const QString &phoneNumber)
{
    ensureFlaggedNumberExists(phoneNumber);

    auto item = m_statItemsLookup.value(phoneNumber);
    if (item->isFavorite) {
        item->isFavorite = false;
        m_favoriteLookup.remove(phoneNumber);
        emit favoriteRemoved(item);
    } else {
        item->isFavorite = true;
        m_favoriteLookup.insert(phoneNumber, item);
        emit favoriteAdded(item);
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

bool NumberStats::ensureFlaggedNumberExists(const QString &phoneNumber)
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
        query.prepare("INSERT INTO contactflags (phonenumber, callcount, isFavorite, isBlocked) "
                      "VALUES (:phoneNumber, 0, 0, 0);");
        query.bindValue(":phoneNumber", phoneNumber);

        if (!query.exec()) {
            qCCritical(lcNumberStats)
                    << "Error on executing SQL query:" << query.lastError().text();
            return false;
        }

        auto statItem = new NumberStat;
        statItem->phoneNumber = phoneNumber;
        statItem->contact = AddressBook::instance().lookupByNumber(phoneNumber);
        m_statItems.append(statItem);
        m_statItemsLookup.insert(phoneNumber, statItem);
        emit numberStatAdded(m_statItems.size() - 1);
        return true;
    }

    return false;
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
    debug.nospace() << "NumberStat(" << statItem.phoneNumber << ", count: " << statItem.callCount
                    << ", is favorite: " << statItem.isFavorite
                    << ", is blocked: " << statItem.isBlocked << ")";
    return debug;
}
