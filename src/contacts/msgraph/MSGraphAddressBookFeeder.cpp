#include "MSGraphAddressBookFeeder.h"
#include "AddressBook.h"
#include "AddressBookManager.h"
#include "ReadOnlyConfdSettings.h"
#include "MSOAuthManager.h"
#include <QLoggingCategory>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequestFactory>
#include <QHttpHeaders>

Q_LOGGING_CATEGORY(msGraphAddressBookFeeder, "gonnect.app.feeder.MSGraphAddressBookFeeder")

MSGraphAddressBookFeeder::MSGraphAddressBookFeeder(const QString &group, const int retryCount,
                                                   const int retryInterval,
                                                   AddressBookManager *parent)
    : QObject(parent),
      m_manager(parent),
      m_group(group),
      m_retryCount(retryCount),
      m_retryInterval(retryInterval)

{
    m_networkAccessManager = new QNetworkAccessManager(this);

    connect(&MSOAuthManager::instance(), &MSOAuthManager::loginSuccessful, this,
            &MSGraphAddressBookFeeder::requestContacts);
}

QUrl MSGraphAddressBookFeeder::networkCheckURL() const
{
    return {};
}

void MSGraphAddressBookFeeder::contactsReceived(QNetworkReply *reply)
{
    if (!reply) {
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(msGraphAddressBookFeeder) << "Error:" << reply->errorString();
        reply->deleteLater();

        Q_EMIT feederFailed();
        return;
    }

    auto jsonText = reply->readAll();
    reply->deleteLater();
    auto doc = QJsonDocument::fromJson(jsonText);
    if (doc.isNull()) {
        return;
    }

    auto value = doc.object()["value"];
    auto contacts = value.toArray();
    auto &addressBook = AddressBook::instance();
    for (const auto &contact : contacts) {
        const auto obj = contact.toObject();

        const auto contactId = obj["id"].toString();
        const auto surName = obj["surname"].toString();
        const auto givenName = obj["givenName"].toString();
        const auto displayName =
                obj["displayName"].toString(); // This is a name that cannot be edited via Outlook,
                                               // in out test Account it holds an old Skype
                                               // Nickname. We use thia as fallback only.
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

        addressBook.addContact(contactId, "", { 1, "Outlook", m_group }, name, company, email,
                               QDateTime(), phoneNumbers, BlockInfo());
    }

    if (doc.object().contains("@odata.nextLink")) {
        QString nextLink = doc.object()["@odata.nextLink"].toString();

        using namespace Qt::StringLiterals;
        QNetworkRequest request(nextLink);
        QHttpHeaders headers;
        headers.append(QHttpHeaders::WellKnownHeader::Authorization,
                       u"Bearer "_s + MSOAuthManager::instance().token());
        request.setHeaders(headers);
        auto *reply = m_networkAccessManager->get(request);
        if (!reply) {
            Q_EMIT feederFailed();
            return;
        }

        connect(reply, &QNetworkReply::finished, reply,
                [this, reply]() { contactsReceived(reply); });
        connect(reply, &QNetworkReply::errorOccurred, this,
                [this, reply](QNetworkReply::NetworkError code) { errorOccurred(reply, code); });
    }
}

void MSGraphAddressBookFeeder::errorOccurred(QNetworkReply *reply, QNetworkReply::NetworkError code)
{
    if (!reply) {
        return;
    }

    switch (code) {
    case QNetworkReply::NoError:
        // Should never happen here
        break;
    // See msgraph API documentation: HTTP status code 401 (Unauthorized) and 403 (Forbidden)
    // may indicate issues with the login.
    case QNetworkReply::AuthenticationRequiredError: // Corresponds to HTTP 401
    case QNetworkReply::ContentAccessDenied: // Corresponds to HTTP 403
        qCCritical(msGraphAddressBookFeeder) << "Network error for msgraph request:" << code
                                             << "requires a new login to account.";
        MSOAuthManager::instance()
                .clearRefreshToken(); // Make sure we don't try to refresh the token again
        break;
    default:
        qCCritical(msGraphAddressBookFeeder) << "Network error for msgraph request:" << code;
        break;
    }

    reply->deleteLater();

    Q_EMIT feederFailed();
}

void MSGraphAddressBookFeeder::requestContacts()
{
    if (!MSOAuthManager::instance().isGranted()) {
        qCWarning(msGraphAddressBookFeeder) << "Cannot request contacts - not logged in";

        Q_EMIT feederFailed();
        return;
    }

    qCDebug(msGraphAddressBookFeeder) << "Requesting contacts with microsoft graph api";

    QNetworkRequestFactory requestFactory({ "https://graph.microsoft.com/v1.0" });
    requestFactory.setBearerToken(MSOAuthManager::instance().token().toLatin1());

    auto request = requestFactory.createRequest("me/contacts");
    auto *reply = m_networkAccessManager->get(request);
    if (!reply) {
        qCCritical(msGraphAddressBookFeeder) << "Failed to create contacts request";

        Q_EMIT feederFailed();
        return;
    }

    connect(reply, &QNetworkReply::finished, reply, [this, reply]() { contactsReceived(reply); });
    connect(reply, &QNetworkReply::errorOccurred, this,
            [this, reply](QNetworkReply::NetworkError code) { errorOccurred(reply, code); });
}

void MSGraphAddressBookFeeder::process()
{
    connect(
            this, &MSGraphAddressBookFeeder::feederFailed, this,
            [this]() {
                // Prepare feeder for re-run
                AddressBook::instance().removeContactsBySource(m_group);

                // Some other error has occurred, wait and try again
                if (m_retryCount > 0) {
                    m_retryCount--;

                    qCCritical(msGraphAddressBookFeeder)
                            << "Failed to process MSGraph sources - trying later";

                    QTimer::singleShot(m_retryInterval, this, [this]() { process(); });
                }
            },
            Qt::SingleShotConnection);

    if (MSOAuthManager::instance().isGranted()) {
        requestContacts();
    } else {
        refreshOrRequestLogin();
    }
}

void MSGraphAddressBookFeeder::refreshOrRequestLogin()
{
    ReadOnlyConfdSettings settings;
    QSet<QByteArray> scopes = {
        { "offline_access" },
#if QT_VERSION < QT_VERSION_CHECK(6, 11, 1)
        { "openid" }, // See QTBUG-145561. Required for Qt <= 6.11 only
#endif
        { "Contacts.Read" },
        { "Contacts.Read.Shared" },
    };

    // If calendar plugin is enabled as well, request calendar permissions as well at
    // the same time
    const auto calendarGroup = QStringLiteral("msgraphcalendar");
    if (settings.childGroups().contains(calendarGroup)
        && settings.value(calendarGroup + QStringLiteral("/enabled"), true).toBool()) {
        scopes.insert({ "Calendars.Read" });
        scopes.insert({ "Calendars.Read.Shared" });
    }

    MSOAuthManager::instance().refreshOrRequestOauthLogin(
            m_group,
            tr("Login to your Microsoft account is required to access your contacts or calendars "
               "from GOnnect."),
            scopes);
}
