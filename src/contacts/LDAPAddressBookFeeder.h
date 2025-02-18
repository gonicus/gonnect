#pragma once

#include <QObject>
#include "IAddressBookFeeder.h"

class LDAPAddressBookFeeder : public QObject, public IAddressBookFeeder
{
    Q_OBJECT

public:

    enum class BindMethod {
        None,
        Simple
    };
    Q_ENUM(BindMethod)

    explicit LDAPAddressBookFeeder(const QString &ldapUrl, const QString &ldapBase,
                                   const QString &ldapFilter,
                                   const BindMethod bindMethod,
                                   const QString& bindDn,
                                   const QString& bindPassword,
                                   QStringList sipStatusSubscriptableAttributes,
                                   const QString &baseNumber = "", QObject *parent = nullptr);

    virtual void feedAddressBook(AddressBook &addressBook) override;

private:
    void clearCStringlist(char **attrs) const;

    BindMethod m_bindMethod = BindMethod::None;
    QString m_bindDn;
    QString m_bindPassword;
    QString m_ldapUrl;
    QString m_ldapBase;
    QString m_ldapFilter;
    QString m_baseNumber;

    QStringList m_sipStatusSubscriptableAttributes;
};
