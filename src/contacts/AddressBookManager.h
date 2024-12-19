#pragma once

#include <QObject>

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

private:
    explicit AddressBookManager(QObject *parent = nullptr);

    void processAddressBookQueue();
    bool processLDAPAddressBookConfig(const QString &group);
    bool processCSVAddressBookConfig(const QString &group);

    QStringList m_addressBookConfigs;
    QStringList m_addressBookQueue;
};
