#pragma once
#include "IAddressBookFactory.h"
#include <QObject>

class AddressBookManager;

class LDAPAddressBookFactory : public QObject, IAddressBookFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "LDAPAddressBookFactory" URI "de.gonicus.gonnect.LDAPAddressBookFactory")
    Q_INTERFACES(IAddressBookFactory)

public:
    LDAPAddressBookFactory() { };

    QString name() const override { return "LDAP"; }

    QStringList configurations() const override;

    IAddressBookFeeder *createFeeder(const QString &id,
                                     AddressBookManager *parent = nullptr) const override;
};
