#pragma once

#include <QObject>
#include <IAddressBookFeeder.h>
#include <QOAuth2AuthorizationCodeFlow>
#include <QNetworkAccessManager>
#include "Contact.h"

class AddressBook;
class AddressBookManager;
class QOAuthHttpServerReplyHandler;

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
    void authStatusChanged(QAbstractOAuth::Status status);

    void contactsReceived(QNetworkReply *reply);

    void requestContacts();

    void authorize();

    void resetFeeder();

    AddressBookManager *m_manager = nullptr;

    QOAuthHttpServerReplyHandler *m_replyHandler = nullptr;

    QString m_group;

    QOAuth2AuthorizationCodeFlow *m_authCodeFlow = nullptr;

    QNetworkAccessManager *m_networkAccessManager = nullptr;

    m_retryCount = 0;
    m_retryInterval = 0;
};
