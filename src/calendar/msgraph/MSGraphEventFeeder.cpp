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

MSGraphEventFeeder::MSGraphEventFeeder(const QString &group, const QDateTime &timeRangeStart,
                                       const QDateTime &timeRangeEnd,
                                       DateEventFeederManager *parent)
    : QObject(parent),
      m_group(group),
      m_manager(parent),
      m_timeRangeStart(timeRangeStart),
      m_timeRangeEnd(timeRangeEnd)
{
    m_networkAccessManager = new QNetworkAccessManager(this);
    m_calendarRefreshTimer.setSingleShot(true);

    connect(&MSOAuthManager::instance(), &MSOAuthManager::loginSuccessful, this, [this]() {
        QTimer::singleShot(std::chrono::seconds(0), this, &MSGraphEventFeeder::requestEvents);
    });
    connect(&m_calendarRefreshTimer, &QTimer::timeout, this, &MSGraphEventFeeder::requestEvents);
}

MSGraphEventFeeder::~MSGraphEventFeeder() { }

QUrl MSGraphEventFeeder::networkCheckURL() const
{
    return {};
}

void MSGraphEventFeeder::init()
{
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
            m_group,
            tr("Login to your Microsoft account is required to access your contacts or calendars "
               "from the GOnnect application."),
            scopes);
}

void MSGraphEventFeeder::eventsReceived(QNetworkReply *reply)
{
    if (!reply) {
        return;
    }
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        return;
    }

    auto jsonText = reply->readAll();
    auto doc = QJsonDocument::fromJson(jsonText);
    if (doc.isNull()) {
        return;
    }

    const QString eventSource = m_group;

    DateEventManager &manager = DateEventManager::instance();
    if (m_isFirstPage) {
        // TODO: In a more advanced variant, we could remove only the events that are no longer in
        //       the response(s) and merely update the rest.
        //       But for now this is good enough. We simply add all events again.
        manager.removeDateEventsBySource(eventSource);
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

        manager.addDateEvent(eventId, eventSource, start, end, subject, location, bodyPreview);
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
    m_calendarRefreshTimer.stop();
    switch (code) {
    case QNetworkReply::NoError:
        // Should never happen here
        break;
    // See msgraph API documentation: HTTP status code 401 (Unauthorized) and 403 (Forbidden)
    // may indicate issues with the login.
    case QNetworkReply::AuthenticationRequiredError: // Corresponds to HTTP 401
    case QNetworkReply::ContentAccessDenied: // Corresponds to HTTP 403
        qCCritical(lcMSGraphEventFeeder)
                << "Network error for msgraph request:" << code << "require new login to account.";
        MSOAuthManager::instance()
                .clearRefreshToken(); // Make sure we don't try to refresh the token again
        refreshOrRequestLogin();
        break;
    default:
        qCCritical(lcMSGraphEventFeeder) << "Network error for msgraph request:" << code;
        break;
    }
    reply->deleteLater();
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
