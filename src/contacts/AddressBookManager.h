#pragma once

#include "SecretResponse.h"

#include <QObject>
#include <QHash>
#include <QTimer>

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
                       std::function<void(SecretResponse response)> callback);

private:
    explicit AddressBookManager(QObject *parent = nullptr);

    QString secret(const QString &group) const;

    void processAddressBookQueue();
    void requeueGroup(const QString &group);
    void scheduleReconnect();

    QTimer m_retryTimer;
    bool m_isProcessing = false;

    QHash<QString, IAddressBookFeeder *> m_addressBookFeeders;

    QStringList m_addressBookConfigs;
    QStringList m_addressBookQueue;
    bool m_reconnectScheduled = false;
    QHash<QString, QMetaObject::Connection> m_viewHelperConnections;
    QMetaObject::Connection m_connectivityConnection;
};
