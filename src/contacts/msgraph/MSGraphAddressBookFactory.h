#pragma once
#include "IAddressBookFactory.h"
#include <QObject>

class AddressBookManager;

class MSGraphAddressBookFactory : public QObject, IAddressBookFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "MSGraphAddressBookFactory" URI
                          "de.gonicus.gonnect.MSGraphAddressBookFactory")
    Q_INTERFACES(IAddressBookFactory)

public:
    MSGraphAddressBookFactory() { };

    QString name() const override { return "MSGraphContacts"; }

    QStringList configurations() const override;

    IAddressBookFeeder *createFeeder(const QString &id, const int retryCount,
                                     const int retryInterval,
                                     AddressBookManager *parent = nullptr) const override;
};
