#pragma once

#include <QObject>
#include "IAddressBookFeeder.h"

class LDAPAddressBookFeeder : public QObject, public IAddressBookFeeder
{
    Q_OBJECT

public:
    enum class BindMethod { None, Simple, GSSAPI };
    Q_ENUM(BindMethod)

    explicit LDAPAddressBookFeeder(bool useSSL, const QString &ldapUrl, const QString &ldapBase,
                                   const QString &ldapFilter, const BindMethod bindMethod,
                                   const QString &bindDn, const QString &bindPassword,
                                   const QString &saslRealm, const QString &saslAuthcid,
                                   const QString &saslAuthzid, const QString &caFilePath,
                                   QStringList sipStatusSubscriptableAttributes,
                                   const QString &baseNumber = "", QObject *parent = nullptr);

    virtual void feedAddressBook(AddressBook &addressBook) override;

private:
    void clearCStringlist(char **attrs) const;

    bool m_useSSL = true;
    BindMethod m_bindMethod = BindMethod::None;
    QString m_bindDn;
    QString m_bindPassword;
    QString m_ldapUrl;
    QString m_ldapBase;
    QString m_ldapFilter;
    QString m_baseNumber;
    QString m_saslRealm;
    QString m_saslAuthcid;
    QString m_saslAuthzid;
    QString m_caFilePath;

    QStringList m_sipStatusSubscriptableAttributes;
};
