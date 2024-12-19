#pragma once

#include <QObject>
#include "IAddressBookFeeder.h"

class LDAPAddressBookFeeder : public QObject, public IAddressBookFeeder
{
    Q_OBJECT

public:
    explicit LDAPAddressBookFeeder(const QString &ldapUrl, const QString &ldapBase,
                                   const QString &ldapFilter,
                                   QStringList sipStatusSubscriptableAttributes,
                                   const QString &baseNumber = "", QObject *parent = nullptr);

    virtual void feedAddressBook(AddressBook &addressBook) override;

private:
    void clearCStringlist(char **attrs) const;

    QString m_ldapUrl;
    QString m_ldapBase;
    QString m_ldapFilter;
    QString m_baseNumber;

    QStringList m_sipStatusSubscriptableAttributes;
};
