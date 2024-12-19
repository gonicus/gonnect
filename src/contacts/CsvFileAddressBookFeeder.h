#pragma once

#include <QObject>
#include "IAddressBookFeeder.h"

class CsvFileAddressBookFeeder : public QObject, IAddressBookFeeder
{
    Q_OBJECT

public:
    explicit CsvFileAddressBookFeeder(const QString &filePath, QObject *parent = nullptr);

    virtual void feedAddressBook(AddressBook &addressBook) override;

private:
    QString m_filePath;
};
