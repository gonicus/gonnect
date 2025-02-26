#pragma once

#include <QObject>
#include <QHash>

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

    QString secret(const QString &group) const;
    QString hashForSettingsGroup(const QString &group) const;

    void processAddressBookQueue();
    bool processLDAPAddressBookConfig(const QString &group);
    bool processLDAPAddressBookConfigImpl(const QString &group, const QString &password);
    bool processCSVAddressBookConfig(const QString &group);
    bool processCardDAVAddressBookConfig(const QString &group);
    void processCardDAVAddressBookConfigImpl(const QString &group, const QString &password);

    QStringList m_addressBookConfigs;
    QStringList m_addressBookQueue;
    QHash<QString, QMetaObject::Connection> m_viewHelperConnections;
};
