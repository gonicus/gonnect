#pragma once

#include <QObject>
#include <QNetworkReply>
#include <IAddressBookFeeder.h>
#include "Contact.h"

class AddressBook;
class AddressBookManager;

class MSGraphAddressBookFeeder : public QObject, public IAddressBookFeeder
{
    Q_OBJECT

public:
    explicit MSGraphAddressBookFeeder(const QString &group, AddressBookManager *parent = nullptr);

    void process() override;
    QUrl networkCheckURL() const override;

private:
    void refreshOrRequestLogin();
    void contactsReceived(QNetworkReply *reply);
    void errorOccurred(QNetworkReply *reply, QNetworkReply::NetworkError code);
    void requestContacts();

    const QString m_group;
    AddressBookManager *m_manager = nullptr;
    QNetworkAccessManager *m_networkAccessManager = nullptr;
};
