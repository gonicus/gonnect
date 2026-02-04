#pragma once

#include <QObject>
#include "IAddressBookFeeder.h"

class AddressBookManager;

class CsvFileAddressBookFeeder : public QObject, public IAddressBookFeeder
{
    Q_OBJECT

public:
    explicit CsvFileAddressBookFeeder(const QString &group, AddressBookManager *parent = nullptr);

    void process() override;

private:
    void feedAddressBook();

    QString m_filePath;
    QString m_group;
    bool m_block = false;
};
