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

    QString name() const override { return "MSGraph"; }

    QStringList configurations() const override;

    IAddressBookFeeder *createFeeder(const QString &id,
                                     AddressBookManager *parent = nullptr) const override;
};
