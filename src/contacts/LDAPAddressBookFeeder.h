#pragma once

#include <QObject>
#include "IAddressBookFeeder.h"
#include "LDAPInitializer.h"

class LDAPAddressBookFeeder : public QObject, public IAddressBookFeeder
{
    Q_OBJECT

public:
    explicit LDAPAddressBookFeeder(const LDAPInitializer::Config &ldapConfig,
                                   QStringList sipStatusSubscriptableAttributes = {},
                                   const QString &baseNumber = "", QObject *parent = nullptr);

    virtual void feedAddressBook(AddressBook &addressBook) override;

private:
    void clearCStringlist(char **attrs) const;

    LDAPInitializer::Config m_ldapConfig;
    QString m_baseNumber;
    QStringList m_sipStatusSubscriptableAttributes;
};
