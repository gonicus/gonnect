#pragma once
#include "IAddressBookFactory.h"
#include <QObject>

class AddressBookManager;

class CardDAVAddressBookFactory : public QObject, IAddressBookFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "CardDAVAddressBookFactory" URI
                          "de.gonicus.gonnect.CardDAVAddressBookFactory")
    Q_INTERFACES(IAddressBookFactory)

public:
    CardDAVAddressBookFactory() { };

    QString name() const override { return "CardDAV"; }

    QStringList configurations() const override;

    IAddressBookFeeder *createFeeder(const QString &id,
                                     AddressBookManager *parent = nullptr) const override;
};
