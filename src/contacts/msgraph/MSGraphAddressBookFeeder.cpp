#include "MSGraphAddressBookFeeder.h"
#include "AddressBook.h"
#include "AddressBookManager.h"
#include "AvatarManager.h"
#include "ReadOnlyConfdSettings.h"
#include <QOAuthHttpServerReplyHandler>
#include <QRegularExpression>
#include <QDesktopServices>
#include <QLoggingCategory>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequestFactory>
#include <QHttpHeaders>

Q_LOGGING_CATEGORY(msGraphAddressBookFeeder, "gonnect.app.feeder.MSGraphAddressBookFeeder")

MSGraphAddressBookFeeder::MSGraphAddressBookFeeder(const QString &group, AddressBookManager *parent)
    : QObject(parent), m_manager(parent), m_group(group)
{
    m_networkAccessManager = new QNetworkAccessManager(this);
}

QUrl MSGraphAddressBookFeeder::networkCheckURL() const
{
    return {};
}

void MSGraphAddressBookFeeder::authStatusChanged(QAbstractOAuth::Status status) {    
    if (status == QAbstractOAuth::Status::Granted) {
        m_replyHandler->close();
        QTimer::singleShot(std::chrono::seconds(0), this, [this]() { 
            requestContacts();
        });
    }
}

void MSGraphAddressBookFeeder::contactsReceived(QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
        return;
    }

    auto jsonText = reply->readAll();
    auto doc = QJsonDocument::fromJson(jsonText);
    if (doc.isNull()) {
        return;
    }

    auto value = doc.object()["value"];
    auto contacts = value.toArray();
    auto& addressBook = AddressBook::instance();
    for (auto contact : contacts) {
        const auto obj = contact.toObject();
        
        const auto contactId = obj["id"].toString();
        const auto surName = obj["surname"].toString();
        const auto givenName = obj["givenName"].toString();
        const auto displayName = obj["displayName"].toString(); // This is a name that cannot be edited via Outlook, in out test Account it holds an old Skype Nickname. We use thia as fallback only.
        const auto nickName = obj["nickName"].toString(); // This one can be edited with Outlook
        const auto emails = obj["emailAddresses"].toArray();
        QString email;
        if (!emails.empty()) {
            email = emails[0].toObject()["address"].toString();
        }
        const auto company = obj["companyName"].toString();

        QString name;
        if (!surName.isEmpty() && !givenName.isEmpty()) {
            name = surName + ", " + givenName;
        } else if (!surName.isEmpty()) {
            name = surName;
        } else if (!nickName.isEmpty()) {
            name = nickName;
        } else if (!givenName.isEmpty()) {
            name = givenName;
        } else {
            name = "Unnamed";
        }

        QList<Contact::PhoneNumber> phoneNumbers;
        if (obj.contains("mobilePhone")) {
            Contact::PhoneNumber number;
            number.type = Contact::NumberType::Mobile;
            number.number = obj["mobilePhone"].toString();
            phoneNumbers.push_back(number);
        }
        if (obj.contains("homePhones")) {
            for (auto phone : obj["homePhones"].toArray()) {
                Contact::PhoneNumber number;
                number.type = Contact::NumberType::Home;
                number.number = phone.toString();
                phoneNumbers.push_back(number);
            }
        }
        if (obj.contains("businessPhones")) {
            for (auto phone : obj["businessPhones"].toArray()) {
                Contact::PhoneNumber number;
                number.type = Contact::NumberType::Commercial;
                number.number = phone.toString();
                phoneNumbers.push_back(number);
            }
        }
        
        addressBook.addContact(contactId, 
                                "", 
                                { 1, "Outlook" }, 
                                name, 
                                company,
                                email,
                                QDateTime(), 
                                phoneNumbers);        
    }

    if (doc.object().contains("@odata.nextLink")) {
        
        QString nextLink = doc.object()["@odata.nextLink"].toString();        

        using namespace Qt::StringLiterals;
        QNetworkRequest request(nextLink);
        QHttpHeaders headers;
        headers.append(QHttpHeaders::WellKnownHeader::Authorization, u"Bearer "_s + m_authCodeFlow->token());
        request.setHeaders(headers);        
        auto *reply = m_networkAccessManager->get(request);
        connect(reply, &QNetworkReply::finished, reply,
                [this, reply]() { contactsReceived(reply); });

    }
}

void MSGraphAddressBookFeeder::requestContacts()
{    
    if (!m_authCodeFlow || m_authCodeFlow->status() != QAbstractOAuth::Status::Granted) {
        qCWarning(msGraphAddressBookFeeder) << "Cannot request contacts - not logged in";
        return;
    }

    qCDebug(msGraphAddressBookFeeder) << "Requesting contacts with microsoft graph api";

    QNetworkRequestFactory requestFactory({ "https://graph.microsoft.com/v1.0" });
    requestFactory.setBearerToken(m_authCodeFlow->token().toLatin1());

    auto request = requestFactory.createRequest("me/contacts");
    auto* reply = m_networkAccessManager->get(request);
    if (!reply) {
        qCCritical(msGraphAddressBookFeeder) << "Failed to create contacts request";
        return;
    }
        
    connect(reply, &QNetworkReply::finished, reply, [this, reply]() {   
        contactsReceived(reply);
    });    
}

void MSGraphAddressBookFeeder::authorize() {
    qCDebug(msGraphAddressBookFeeder) << "Starting microsoft authorization";
    if (!m_replyHandler) {
        m_replyHandler = new QOAuthHttpServerReplyHandler(this);
    }

    if (!m_replyHandler->isListening()) {
        // For each port a redirect url must be registered in the azure portal for this app.
        // http://127.0.0.1:33221 ...
        std::array<uint16_t, 3> availablePorts = { 33221, 34221, 33521 };

        // try and hope that one of the available ports is available for listening
        for (auto port : availablePorts) {
            if (m_replyHandler->listen(QHostAddress::Any, port)) {
                break;
            }
        }

        if (!m_replyHandler->isListening()) {
            qCCritical(msGraphAddressBookFeeder)
                    << "Failed to start QOAuthHttpServerReplyHandler. No port available?";
            return;
        }
    }

    if (!m_authCodeFlow) {
        const QSet<QByteArray> tokens = { { "Contacts.Read" } };
        m_authCodeFlow = new QOAuth2AuthorizationCodeFlow(this);
        m_authCodeFlow->setReplyHandler(m_replyHandler);
        m_authCodeFlow->setAuthorizationUrl(
                QStringLiteral("https://login.microsoftonline.com/common/oauth2/v2.0/authorize"));
        m_authCodeFlow->setTokenUrl(
                { "https://login.microsoftonline.com/common/oauth2/v2.0/token" });
        m_authCodeFlow->setRequestedScopeTokens(tokens);
        m_authCodeFlow->setClientIdentifier(QStringLiteral("040bd189-3f48-414d-acf8-076bf1983326"));

        connect(m_authCodeFlow, &QOAuth2AuthorizationCodeFlow::statusChanged, this,
                &MSGraphAddressBookFeeder::authStatusChanged);

        connect(m_authCodeFlow, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser,
                &QDesktopServices::openUrl);
    }

    m_authCodeFlow->grant();
}

void MSGraphAddressBookFeeder::process()
{
    if (m_authCodeFlow
        && m_authCodeFlow->status() == QAbstractOAuth::Status::Granted) {
        requestContacts();
    } else {
        authorize();
    }
}

