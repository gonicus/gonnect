#pragma once

#include <QObject>
#include <QHash>
#include <QMutex>

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

    void acquireSecret(const QString &group, std::function<void(const QString &secret)> callback);

private:
    explicit AddressBookManager(QObject *parent = nullptr);

    QString secret(const QString &group) const;
    QMutex m_queueMutex;

    void processAddressBookQueue();

    QHash<QString, IAddressBookFeeder *> m_addressBookFeeders;

    QStringList m_addressBookConfigs;
    QStringList m_addressBookQueue;
    QHash<QString, QMetaObject::Connection> m_viewHelperConnections;
};
