#pragma once

#include <QObject>
#include <QHash>
#include <QByteArray>
#include "IAddressBookFeeder.h"
#include "LDAPInitializer.h"
#include "Contact.h"
#include "BlockInfo.h"

class AddressBookManager;

class LDAPAddressBookFeeder : public QObject, public IAddressBookFeeder
{
    Q_OBJECT

public:
    explicit LDAPAddressBookFeeder(const QString &group, const int retryCount,
                                   const int retryInterval, AddressBookManager *parent = nullptr);

    void process() override;
    QUrl networkCheckURL() const override;

Q_SIGNALS:
    /// Private signal
    void newContactReady(const QString &dn, const QString &sourceUid,
                         const Contact::ContactSourceInfo &contactSourceInfo, const QString &name,
                         const QString &company, const QString &mail, const QDateTime &lastModified,
                         const QList<Contact::PhoneNumber> &phoneNumbers, QPrivateSignal);

    /// Private signal
    void newExternalImageAdded(const QString &id, const QByteArray &data, const QDateTime &modified,
                               QPrivateSignal);

    void feederFailed();

private:
    void clearCStringlist(char **attrs) const;
    void init(const LDAPInitializer::Config &ldapConfig,
              QStringList sipStatusSubscriptableAttributes = {}, const QString &baseNumber = "");
    void feedAddressBook();
    void loadAvatars(const QList<const Contact *> &contacts);
    void loadAllAvatars(const LDAPInitializer::Config &ldapConfig);
    void processImpl(const QString &password);
    void processResult(LDAPMessage *ldapMessage);
    void startContactQuery();

    void resetFeeder();

    // Per-account mapping from semantic contact roles to the LDAP attribute
    // names actually published by the directory. Empty entries disable that
    // role for the current source. Defaults match standard inetOrgPerson.
    struct AttributeMap
    {
        QByteArray name;
        QByteArray uid;
        QByteArray company;
        QByteArray email;
        QByteArray commercial;
        QByteArray mobile;
        QByteArray home;
        QByteArray avatar;
    };

    LDAPInitializer::Config m_ldapConfig;

    AddressBookManager *m_manager = nullptr;

    LDAP *m_ldap = nullptr;
    int m_ldapSearchMessageId = -1;
    QString m_group;
    QString m_baseNumber;
    QStringList m_sipStatusSubscriptableAttributes;
    BlockInfo m_blockInfo;
    AttributeMap m_attrs;

    bool m_authFailed = false;
    int m_retryCount = 0;
    int m_retryInterval = 0;
};
