#include "MSGraphEventFeeder.h"
#include "DateEventManager.h"
#include "DateEventFeederManager.h"
#include "ReadOnlyConfdSettings.h"
#include "MSOAuthManager.h"

#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequestFactory>
#include <QHttpHeaders>

Q_LOGGING_CATEGORY(lcMSGraphEventFeeder, "gonnect.app.dateevents.feeder.MSGraphEventFeeder")

MSGraphEventFeeder::MSGraphEventFeeder(const QString &source, const QDateTime &timeRangeStart,
                                       const QDateTime &timeRangeEnd, const int retryCount,
                                       const int retryInterval, DateEventFeederManager *parent)
    : QObject(parent),
      m_source(source),
      m_timeRangeStart(timeRangeStart),
      m_timeRangeEnd(timeRangeEnd),
      m_manager(parent),
      m_retryCount(retryCount),
      m_retryInterval(retryInterval)
{
    m_networkAccessManager = new QNetworkAccessManager(this);
    m_calendarRefreshTimer.setSingleShot(true);

    connect(&MSOAuthManager::instance(), &MSOAuthManager::loginSuccessful, this,
            &MSGraphEventFeeder::requestEvents);
    connect(&m_calendarRefreshTimer, &QTimer::timeout, this, &MSGraphEventFeeder::requestEvents);
}

MSGraphEventFeeder::~MSGraphEventFeeder() { }

QUrl MSGraphEventFeeder::networkCheckURL() const
{
    return {};
}

void MSGraphEventFeeder::init()
{
    connect(
            this, &MSGraphEventFeeder::feederFailed, this,
            [this]() {
                // Prepare feeder for re-run
                m_calendarRefreshTimer.stop();
                m_isFirstPage = false;
                DateEventManager::instance().removeDateEventsBySource(m_source);

                if (m_retryCount > 0) {
                    m_retryCount--;

                    qCWarning(lcMSGraphEventFeeder)
                            << "Failed to process MSGraph sources - trying later";

                    // Retry
                    QTimer::singleShot(m_retryInterval, this, [this]() { init(); });
                }
            },
            Qt::SingleShotConnection);

    if (MSOAuthManager::instance().isGranted()) {
        requestEvents();
    } else {
        refreshOrRequestLogin();
    }
}

void MSGraphEventFeeder::refreshOrRequestLogin()
{
    ReadOnlyConfdSettings settings;
    QSet<QByteArray> scopes = {
        { "offline_access" },
#if QT_VERSION < QT_VERSION_CHECK(6, 11, 1)
        { "openid" }, // See QTBUG-145561. Required for Qt <= 6.11 only
#endif
        { "Calendars.Read" },
        { "Calendars.Read.Shared" },
    };

    // If contacts plugin is enabled as well, request calendar permissions as well at
    // the same time
    const auto contactsGroup = QStringLiteral("msgraphcontacts");
    if (settings.childGroups().contains(contactsGroup)
        && settings.value(contactsGroup + QStringLiteral("/enabled"), true).toBool()) {
        scopes.insert({ "Contacts.Read" });
        scopes.insert({ "Contacts.Read.Shared" });
    }

    MSOAuthManager::instance().refreshOrRequestOauthLogin(
            m_source,
            tr("Login to your Microsoft account is required to access your contacts or calendars "
               "from GOnnect."),
            scopes);
}

void MSGraphEventFeeder::eventsReceived(QNetworkReply *reply)
{
    if (!reply) {
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(lcMSGraphEventFeeder) << "Error:" << reply->errorString();
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

    DateEventManager &manager = DateEventManager::instance();
    if (m_isFirstPage) {
        manager.removeDateEventsBySource(m_source);
    }

    auto value = doc.object()["value"];
    auto events = value.toArray();
    for (const auto &event : events) {
        const auto obj = event.toObject();
        if (!obj.contains("id") || !obj.contains("start") || !obj.contains("end")
            || !obj.contains("subject")) {
            qCWarning(lcMSGraphEventFeeder) << "Invalid event format encountered";
            continue;
        }
        if (obj.contains("isCancelled") && obj["isCancelled"].toBool()) {
            continue;
        }
        const auto eventId = obj["id"].toString();
        const QDateTime start = parseDateTime(obj["start"]);
        const QDateTime end = parseDateTime(obj["end"]);
        if (!start.isValid() || !end.isValid()) {
            qCCritical(lcMSGraphEventFeeder)
                    << "Error parsing start or end datetime for event" << eventId
                    << "start:" << obj["start"].toString() << "end:" << obj["end"].toString();
            continue;
        }
        const auto subject = obj["subject"].toString();
        QString location;
        if (obj.contains("location") && obj["location"].isObject()) {
            location = obj["location"].toObject()["displayName"].toString();
        }
        // Alternatively,we could use obj["body"].toObject()["content"].toString(), however this
        // is likely in HTML (see obj["body"].toObject()["contentType"].toString())
        const auto bodyPreview = obj["bodyPreview"].toString();

        manager.addDateEvent(eventId, m_source, start, end, subject, location, bodyPreview);
    }

    m_isFirstPage = false;
    if (doc.object().contains("@odata.nextLink")) {
        QString nextLink = doc.object()["@odata.nextLink"].toString();

        using namespace Qt::StringLiterals;
        QNetworkRequest request(nextLink);
        QHttpHeaders headers;
        headers.append(QHttpHeaders::WellKnownHeader::Authorization,
                       u"Bearer "_s + MSOAuthManager::instance().token());
        request.setHeaders(headers);
        auto *reply = m_networkAccessManager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() { eventsReceived(reply); });
        connect(reply, &QNetworkReply::errorOccurred, this,
                [this, reply](QNetworkReply::NetworkError code) { errorOccurred(reply, code); });
    } else {
        ReadOnlyConfdSettings settings;
        m_calendarRefreshTimer.setInterval(settings.value("interval", 900000).toInt());
        m_calendarRefreshTimer.start();
    }
}

void MSGraphEventFeeder::requestEvents()
{
    m_calendarRefreshTimer.stop();
    if (!MSOAuthManager::instance().isGranted()) {
        qCWarning(lcMSGraphEventFeeder) << "Cannot request events - not logged in";

        Q_EMIT feederFailed();
        return;
    }

    if (!m_timeRangeStart.isValid() || !m_timeRangeEnd.isValid()) {
        qCWarning(lcMSGraphEventFeeder) << "Cannot request events - timerange not valid";
        return;
    }

    qCDebug(lcMSGraphEventFeeder) << "Requesting events with microsoft graph api";

    QNetworkRequestFactory requestFactory({ "https://graph.microsoft.com/v1.0" });
    requestFactory.setBearerToken(MSOAuthManager::instance().token().toLatin1());

    // NOTE: timerange is required for calendarview requests. Must be in ISO-8601 format
    //       ("2026-04-21T14:21:51.565Z")
    auto request = requestFactory.createRequest(QStringLiteral("me/calendarview?startdatetime=")
                                                + m_timeRangeStart.toString(Qt::ISODate)
                                                + QStringLiteral("&enddatetime=")
                                                + m_timeRangeEnd.toString(Qt::ISODate));
    auto *reply = m_networkAccessManager->get(request);
    if (!reply) {
        qCCritical(lcMSGraphEventFeeder) << "Failed to create contacts request";

        Q_EMIT feederFailed();
        return;
    }

    m_isFirstPage = true;
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { eventsReceived(reply); });
    connect(reply, &QNetworkReply::errorOccurred, this,
            [this, reply](QNetworkReply::NetworkError code) { errorOccurred(reply, code); });
}

void MSGraphEventFeeder::errorOccurred(QNetworkReply *reply, QNetworkReply::NetworkError code)
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
        qCCritical(lcMSGraphEventFeeder) << "Network error for msgraph request:" << code
                                         << "requires a new login to account.";
        MSOAuthManager::instance()
                .clearRefreshToken(); // Make sure we don't try to refresh the token again
        break;
    default:
        qCCritical(lcMSGraphEventFeeder) << "Network error for msgraph request:" << code;
        break;
    }

    reply->deleteLater();

    Q_EMIT feederFailed();
}

QDateTime MSGraphEventFeeder::parseDateTime(const QJsonValue &dateTimeContainer)
{
    if (!dateTimeContainer.isObject()) {
        return QDateTime();
    }
    const auto obj = dateTimeContainer.toObject();
    // Container have a "dateTime" and a "timeZone" key.
    // We only need dateTime: timeZone is UTC in responses, if nothing else was requested.
    if (!obj.contains("dateTime")) {
        return QDateTime();
    }
    auto s = obj["dateTime"].toString();
    // Epected format: "2026-03-20T17:00:00.0000000"
    // we only need up to 3 digits after the decimal point (=ms precision) and we need a 'Z'
    // afterwards, for Qt to recognize UTC.
    if (s.length() < 23) {
        return QDateTime();
    }
    s = s.left(23) + QStringLiteral("Z");
    return QDateTime::fromString(s, QStringLiteral("yyyy-MM-ddTHH:mm:ss.zzzt"));
}
