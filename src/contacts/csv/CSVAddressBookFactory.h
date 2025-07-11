#pragma once
#include "IAddressBookFactory.h"
#include <QObject>

class AddressBookManager;

class CSVAddressBookFactory : public QObject, IAddressBookFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "CSVAddressBookFactory" URI "de.gonicus.gonnect.CSVAddressBookFactory")
    Q_INTERFACES(IAddressBookFactory)

public:
    CSVAddressBookFactory() { };

    QString name() const override { return "CSV"; }

    QStringList configurations() const override;

    IAddressBookFeeder *createFeeder(const QString &id,
                                     AddressBookManager *parent = nullptr) const override;
};
