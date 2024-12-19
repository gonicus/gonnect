#pragma once

#include <QObject>
#include <QDateTime>

#include "CallHistoryItem.h"
#include "PhoneNumberUtil.h"

class CallHistory : public QObject
{
    Q_OBJECT

public:
    static CallHistory &instance()
    {
        static CallHistory *_instance = nullptr;
        if (!_instance) {
            _instance = new CallHistory;
        }
        return *_instance;
    }

    CallHistoryItem *addHistoryItem(CallHistoryItem *item);
    CallHistoryItem *addHistoryItem(CallHistoryItem::Type type, const QString &account,
                                    const QString &remoteUrl, const QString &contactId = "",
                                    bool isSipSubscriptable = false);
    QList<CallHistoryItem *> historyItems() const { return m_historyItems; };
    qsizetype indexOfItem(const CallHistoryItem *item) const
    {
        return m_historyItems.indexOf(item);
    }

    void writeToDatabase(CallHistoryItem &item);
    void readFromDatabase();

    /// Returns info object about the last that was outgoing; empty object if could not be
    /// determined
    ContactInfo lastOutgoingSipInfo() const;

private:
    explicit CallHistory(QObject *parent = nullptr);
    qsizetype insertItemAtCorrectPosition(CallHistoryItem *item);
    void ensureDatabaseVersion();

    QList<CallHistoryItem *> m_historyItems;
    QString m_databasePath;

signals:
    void itemAdded(qsizetype index, CallHistoryItem *item);
    void dataChanged(qsizetype index, CallHistoryItem *item);
};

QDebug operator<<(QDebug debug, const CallHistory &history);
