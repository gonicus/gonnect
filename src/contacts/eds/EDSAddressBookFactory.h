#pragma once

#include "EDSAddressBookFeeder.h"
#include "IAddressBookFactory.h"

#include <QObject>

class AddressBookManager;

class EDSAddressBookFactory : public QObject, IAddressBookFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "EDSAddressBookFactory" URI "de.gonicus.gonnect.EDSAddressBookFactory")
    Q_INTERFACES(IAddressBookFactory)

public:
    EDSAddressBookFactory() { };

    QString name() const override { return "EDS"; }

    QStringList configurations() const override;

    IAddressBookFeeder *createFeeder(const QString &id,
                                     AddressBookManager *parent = nullptr) const override;
};
