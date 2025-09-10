#pragma once

#include <QObject>
#include <Akonadi/ContactSearchJob>
#include <Akonadi/Monitor>
#include "IAddressBookFeeder.h"

class AddressBookManager;

class AkonadiAddressBookFeeder : public QObject, public IAddressBookFeeder
{
    Q_OBJECT

public:
    explicit AkonadiAddressBookFeeder(const QString &group, AddressBookManager *parent = nullptr);

    virtual void process() override;
    ~AkonadiAddressBookFeeder();

private Q_SLOTS:
    void processSearchResult(KJob *);

private:
    QString m_group;
    QString m_filePath;

    Akonadi::Session *m_session = nullptr;
    Akonadi::Monitor *m_monitor = nullptr;
    Akonadi::ContactSearchJob *m_job = nullptr;
};
