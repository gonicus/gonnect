#pragma once

#include <QObject>
#include <QHash>
#include "IAddressBookFeeder.h"
#include "LDAPInitializer.h"
#include "Contact.h"
#include "BlockInfo.h"

class AddressBookManager;

class LDAPAddressBookFeeder : public QObject, public IAddressBookFeeder
{
    Q_OBJECT

public:
    explicit LDAPAddressBookFeeder(const QString &group, AddressBookManager *parent = nullptr);

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

    LDAPInitializer::Config m_ldapConfig;

    AddressBookManager *m_manager = nullptr;

    LDAP *m_ldap = nullptr;
    int m_ldapSearchMessageId = -1;
    QString m_group;
    QString m_baseNumber;
    QStringList m_sipStatusSubscriptableAttributes;
    BlockInfo m_blockInfo;
};
