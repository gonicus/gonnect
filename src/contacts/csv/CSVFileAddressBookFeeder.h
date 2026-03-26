#pragma once

#include <QObject>
#include "IAddressBookFeeder.h"
#include "BlockInfo.h"

class AddressBookManager;

class CsvFileAddressBookFeeder : public QObject, public IAddressBookFeeder
{
    Q_OBJECT

public:
    explicit CsvFileAddressBookFeeder(const QString &group, const int retryCount,
                                      const int retryInterval,
                                      AddressBookManager *parent = nullptr);

    void process() override;

private:
    void feedAddressBook();

    QString m_filePath;
    QString m_group;
    BlockInfo m_blockInfo;
};
