#pragma once

#include <QObject>
#include <QHash>
#include "IAddressBookFeeder.h"
#include "LDAPInitializer.h"

class AddressBookManager;

class LDAPAddressBookFeeder : public QObject, public IAddressBookFeeder
{
    Q_OBJECT

public:
    explicit LDAPAddressBookFeeder(const QString &group, AddressBookManager *parent = nullptr);

    void process() override;
    QUrl networkCheckURL() const override;

private:
    void clearCStringlist(char **attrs) const;
    void init(const LDAPInitializer::Config &ldapConfig,
              QStringList sipStatusSubscriptableAttributes = {}, const QString &baseNumber = "");
    void feedAddressBook();
    void processImpl(const QString &password);

    LDAPInitializer::Config m_ldapConfig;

    AddressBookManager *m_manager = nullptr;

    QString m_group;
    QString m_baseNumber;
    QStringList m_sipStatusSubscriptableAttributes;
};
