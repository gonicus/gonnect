#pragma once

#include <QObject>
#include <QHash>
#include <QMutex>
#include <QTimer>
#include <atomic>

class IAddressBookFeeder;

class AddressBookManager : public QObject
{
    Q_OBJECT

public:
    static AddressBookManager &instance()
    {
        static AddressBookManager *_instance = nullptr;
        if (!_instance) {
            _instance = new AddressBookManager;
        }
        return *_instance;
    };

    void initAddressBookConfigs();
    void reloadAddressBook();

    static QString hashForSettingsGroup(const QString &group);

    void acquireSecret(bool forcePrompt, const QString &group,
                       std::function<void(const QString &secret)> callback);

private:
    explicit AddressBookManager(QObject *parent = nullptr);

    QString secret(const QString &group) const;
    QMutex m_queueMutex;
    std::atomic<int> m_remainingMutexLockTries = 10;

    void processAddressBookQueue();
    void requeueGroup(const QString &group);
    void scheduleReconnect();
    QTimer m_retryTimer;

    QHash<QString, IAddressBookFeeder *> m_addressBookFeeders;

    QStringList m_addressBookConfigs;
    QStringList m_addressBookQueue;
    bool m_reconnectScheduled = false;
    QHash<QString, QMetaObject::Connection> m_viewHelperConnections;
    QMetaObject::Connection m_connectivityConnection;
};
