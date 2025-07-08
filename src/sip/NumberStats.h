#pragma once

#include <QObject>
#include <QHash>
#include <QTimer>
#include <qqmlintegration.h>

struct NumberStat;

class NumberStats : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    enum class ContactType { PhoneNumber, JitsiMeetUrl };
    Q_ENUM(ContactType)

    virtual ~NumberStats();

    static NumberStats &instance()
    {
        static NumberStats *_instance = nullptr;
        if (!_instance) {
            _instance = new NumberStats;
        }
        return *_instance;
    };

    void incrementCallCount(const QString &phoneNumber);

    QList<NumberStat *> statsAndFlags() const { return m_statItems; };
    QList<NumberStat *> favorites() const;

    QStringList mostCalled(quint8 limit, bool includeFavorites = true) const;

    bool isFavorite(const QString &phoneNumber) const
    {
        return m_favoriteLookup.contains(phoneNumber);
    }
    void toggleFavorite(const QString &phoneNumber, const NumberStats::ContactType contactType);

    const NumberStat *numberStat(const QString &phoneNumber) const
    {
        return m_statItemsLookup.value(phoneNumber, nullptr);
    }

private:
    explicit NumberStats(QObject *parent = nullptr);
    void initialRead();

    /**
     * @brief ensureFlaggedNumberExists creates a new entry in flagged table in database, if it does
     * not exist already.
     * @param phoneNumber
     * @param contactType
     * @return Whether an appropriate line exists in the database afterwards
     */
    bool ensureFlaggedNumberExists(
            const QString &phoneNumber,
            const NumberStats::ContactType contactType = NumberStats::ContactType::PhoneNumber);

    QHash<QString, NumberStat *> m_statItemsLookup;
    QHash<QString, NumberStat *> m_favoriteLookup;
    QList<NumberStat *> m_statItems;
    QTimer m_debounceAddressBookUpdateTimer;

signals:
    void countChanged(qsizetype index);
    void numberStatAdded(qsizetype index);
    void favoriteAdded(NumberStat *item);
    void favoriteRemoved(NumberStat *item);
    void modelReset();
};

QDebug operator<<(QDebug debug, const NumberStats &stats);
QDebug operator<<(QDebug debug, const NumberStat &statItem);
