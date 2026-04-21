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
    explicit MSGraphAddressBookFeeder(const QString &group, const int retryCount,
                                      const int retryInterval,
                                      AddressBookManager *parent = nullptr);

    void process() override;
    QUrl networkCheckURL() const override;

Q_SIGNALS:
    void feederFailed();

private:
    void refreshOrRequestLogin();
    void contactsReceived(QNetworkReply *reply);
    void errorOccurred(QNetworkReply *reply, QNetworkReply::NetworkError code);
    void requestContacts();

    void authorize();

    void resetFeeder();

    AddressBookManager *m_manager = nullptr;
    QNetworkAccessManager *m_networkAccessManager = nullptr;

    const QString m_group;
    int m_retryCount = 0;
    int m_retryInterval = 0;
};
