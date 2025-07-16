#pragma once
#include <QtCore/QString>
#include <QtCore/QtPlugin>
#include "IAddressBookFeeder.h"

#define IAddressBookFactory_iid "de.gonicus.gonnect.IAddressBookFactory"

class AddressBookManager;

class IAddressBookFactory
{
public:
    virtual QString name() const = 0;

    virtual QStringList configurations() const = 0;
    virtual IAddressBookFeeder *createFeeder(const QString &id,
                                             AddressBookManager *parent = nullptr) const = 0;

    virtual ~IAddressBookFactory() = default;
};

Q_DECLARE_INTERFACE(IAddressBookFactory, IAddressBookFactory_iid)
