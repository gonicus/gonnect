#pragma once

#include <atomic>
#include <functional>
#include <QObject>
#include <QHash>
#include <QByteArray>
#include <QThread>
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

    ~LDAPAddressBookFeeder() override;

Q_SIGNALS:
    /// Private signal
    void newContactReady(const QString &dn, const QString &sourceUid,
                         const Contact::ContactSourceInfo &contactSourceInfo, const QString &name,
                         const QString &company, const QString &mail, const QString &lastModified,
                         const QList<Contact::PhoneNumber> &phoneNumbers, QPrivateSignal);

    /// Private signal
    void newExternalImageAdded(const QString &dn, const QByteArray &data, const QString &modified,
                               QPrivateSignal);

    /// Private signal
    void errorOccurred(const QString &message, QPrivateSignal);

    void feederFailed();

private:
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

    // Captures query information required when running in threads
    struct QueryContext
    {
        LDAPInitializer::Config config;
        AttributeMap attrs;
        QString baseNumber;
        QStringList sipStatusSubscriptableAttributes;
        Contact::ContactSourceInfo sourceInfo;
        int pageSize = 500;
    };

    void clearCStringlist(char **attrs) const;
    char **toCStringList(const QList<QByteArray> &values) const;

    void init(const LDAPInitializer::Config &ldapConfig,
              QStringList sipStatusSubscriptableAttributes = {}, const QString &baseNumber = "");
    void loadAvatarsForContacts();
    void loadAvatars(const QList<const Contact *> &contacts);
    void loadAllAvatars(const LDAPInitializer::Config &ldapConfig);

    void processImpl(const QString &password);

    QueryContext createQueryContext() const;

    bool pagedSearch(LDAP *ldap, const QueryContext &ctx, char **attrs,
                     const std::function<void(LDAP *, LDAPMessage *)> &onEntry);
    void parseContactEntry(LDAP *ldap, LDAPMessage *entry, const QueryContext &ctx);
    void parseAvatarEntry(LDAP *ldap, LDAPMessage *entry, const QByteArray &avatarAttr);

    void startContactQuery();
    bool startQueryThread(std::function<bool()> work, std::function<void(bool)> onFinished);
    bool runQuery(const QueryContext &ctx, const QList<QByteArray> &attributes,
                  std::atomic<bool> *authFailed,
                  const std::function<void(LDAP *, LDAPMessage *)> &onEntry);

    void resetFeeder();

    QDateTime parseLDAPTimestamp(const QString &timestamp) const;

    LDAPInitializer::Config m_ldapConfig;

    AddressBookManager *m_manager = nullptr;

    QThread *m_worker = nullptr;

    QString m_group;
    QString m_baseNumber;
    QStringList m_sipStatusSubscriptableAttributes;
    BlockInfo m_blockInfo;
    AttributeMap m_attrs;

    bool m_authFailed = false;
    int m_retryCount = 0;
    int m_retryInterval = 0;
    int m_pageSize = 500;
};
