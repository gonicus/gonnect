#pragma once
#include "IAddressBookFactory.h"
#include <QObject>

class AddressBookManager;

class AkonadiAddressBookFactory : public QObject, IAddressBookFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "AkonadiAddressBookFactory" URI
                          "de.gonicus.gonnect.AkonadiAddressBookFactory")
    Q_INTERFACES(IAddressBookFactory)

public:
    AkonadiAddressBookFactory() { };

    QString name() const override { return "Akonadi"; }

    QStringList configurations() const override;

    IAddressBookFeeder *createFeeder(const QString &id,
                                     AddressBookManager *parent = nullptr) const override;
};
