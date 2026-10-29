#include <QMap>
#include <QLoggingCategory>
#include <QRegularExpression>

#include "CalDAVEventFeeder.h"
#include "DateEventManager.h"
#include "SecretResponse.h"
#include "ErrorBus.h"
#include "WebDavClient.h"

Q_LOGGING_CATEGORY(lcCalDAVEventFeeder, "gonnect.app.dateevents.feeder.caldav")

using namespace std::chrono_literals;

CalDAVEventFeeder::CalDAVEventFeeder(QObject *parent, const CalDAVEventFeederConfig &config)
    : QObject(parent), m_config(config)
{
}

CalDAVEventFeeder::~CalDAVEventFeeder()
{
    if (m_calendarRefreshTimer.isActive()) {
        m_calendarRefreshTimer.stop();
    }
}

QUrl CalDAVEventFeeder::networkCheckURL() const
{
    return (QUrl(QString("%1://%2%3")
                         .arg(m_config.useSSL ? "https" : "http", m_config.host, m_config.path)));
}

void CalDAVEventFeeder::init()
{
    if (!m_areWebDavConnectionsInitialized) {
        m_areWebDavConnectionsInitialized = true;

        /*
            As the Kopano CalDAV server doesn't provide 'getetag' and 'getlastmodified' values,
            it's impossible to actively poll for changes. Thus, we're simply re-launching the
            process() loop, at least until there's a better solution.

            Kopano saves every VEVENT wrapped in a VCALENDAR entry in separate '.ics' files in
            '/caldav/<USER>/Kalender'. A full calendar is generated on the fly once requested.
        */
        m_calendarRefreshTimer.setInterval(m_config.interval);
        connect(&m_calendarRefreshTimer, &QTimer::timeout, this, [this]() { run(); });
    }

    m_calendarRefreshTimer.start();

    run();
}

void CalDAVEventFeeder::resetCalendar()
{
    DateEventManager &manager = DateEventManager::instance();

    for (auto &concreteSource : std::as_const(m_concreteSources)) {
        manager.removeDateEventsBySource(concreteSource);
    }
}

QCoro::Task<> CalDAVEventFeeder::run()
{
    if (m_isRunning) {
        qCInfo(lcCalDAVEventFeeder) << "Already running - ignoring run() call";
        co_return;
    }
    qCDebug(lcCalDAVEventFeeder) << "Starting run() of CalDAV feeder" << m_config.source;

    m_isRunning = true;
    bool forcePrompt = false;
    int retriesLeft = m_config.retryCount;

    // Retry loop within one sync attempt
    while (true) {

        // Retrieve secret
        const auto secret = co_await DateEventFeederManager::instance().acquireSecret(
                forcePrompt, m_config.source);

        qCDebug(lcCalDAVEventFeeder) << "Secret acquired, hasError=" << secret.hasError;

        if (secret.hasError) {
            qCCritical(lcCalDAVEventFeeder)
                    << "Authentication for" << m_config.source << "has failed";
            ErrorBus::instance().addError(tr("Authentication error for %1").arg(m_config.source));
            m_isRunning = false;
            co_return;
        }

        // Create WebDAV client
        const QUrl webdavUrl(QString("%1://%2:%3%4")
                                     .arg(m_config.useSSL ? "https" : "http", m_config.host,
                                          QString::number(m_config.port), m_config.path));
        m_webdavClient = std::make_unique<WebDavClient>(
                WebDavClient::ConnectionConfig{ webdavUrl, m_config.user, secret.secret });

        qCDebug(lcCalDAVEventFeeder) << "Created WebDavClient with url" << webdavUrl.toString();

        // Get WebDAV root entry
        auto *reply = co_await m_webdavClient->propfind("", 1,
                                                        {
                                                                { "D", "DAV:", "resourcetype" },
                                                                { "D", "DAV:", "getetag" },
                                                                { "D", "DAV:", "getlastmodified" },
                                                                { "D", "DAV:", "displayname" },
                                                        });

        qCDebug(lcCalDAVEventFeeder) << "PROPFIND returned, error=" << reply->error();

        // Auth incorrect
        if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
            reply->deleteLater();
            qCWarning(lcCalDAVEventFeeder) << "Failed to process CalDAV source - invalid password";
            forcePrompt = true;
            continue;
        }

        // Other error
        if (reply->error() != QNetworkReply::NoError) {
            reply->deleteLater();

            if (retriesLeft-- <= 0) {
                qCCritical(lcCalDAVEventFeeder) << "Repeatedly failed to access CalDAV source"
                                                << m_config.source << "- retries exhausted";
                ErrorBus::instance().addError(
                        tr("CalDAV source %1 repeatedly failed").arg(m_config.source));
                m_isRunning = false;
                co_return;
            }

            qCWarning(lcCalDAVEventFeeder)
                    << "Failed to process CalDAV source" << m_config.source << "-"
                    << reply->errorString() << "- trying again after" << m_config.retryInterval
                    << "ms," << retriesLeft << "retries left";
            co_await QCoro::sleepFor(std::chrono::milliseconds(m_config.retryInterval));
            continue;
        }

        // Process correct result - traverse items
        const auto items = WebDavPropfindParser{}.parseMultiStatus(reply->readAll());
        reply->deleteLater();

        qCDebug(lcCalDAVEventFeeder) << "Received" << items.size() << "items";

        resetCalendar();
        m_checksums.clear();

        uint successCount = 0;
        uint failCount = 0;

        for (const auto &item : items) {
            if (item.isCollection()) {
                continue;
            }

            auto *itemReply = co_await m_webdavClient->sendRequest(item.href, "GET", {});
            if (itemReply->error() == QNetworkReply::NoError) {
                const auto data = itemReply->readAll();
                if (responseDataChanged(data)) {
                    const bool success = processResponse(
                            data, QString("%1_%2").arg(m_config.source, item.name()));

                    if (success) {
                        ++successCount;
                    } else {
                        ++failCount;
                    }
                }
            } else {
                qCWarning(lcCalDAVEventFeeder)
                        << "GET failed for" << item.href << "-" << itemReply->errorString();
            }

            itemReply->deleteLater();
        }

        qCDebug(lcCalDAVEventFeeder)
                << "Processed" << items.size() << "items, successful:" << successCount
                << "failed:" << failCount;

        break;
    }

    m_isRunning = false;
}

bool CalDAVEventFeeder::processResponse(const QByteArray &data, const QString &source)
{
    DateEventManager &manager = DateEventManager::instance();

    QMap<QString, QList<QDateTime>> exdatesById;

    icalcomponent *calendar = icalparser_parse_string(data.toStdString().data());
    if (calendar) {
        // VEVENT's
        for (icalcomponent *event =
                     icalcomponent_get_first_component(calendar, ICAL_VEVENT_COMPONENT);
             event != nullptr;
             event = icalcomponent_get_next_component(calendar, ICAL_VEVENT_COMPONENT)) {
            QString id = icalcomponent_get_uid(event);

            icaltimetype dtstart = icalcomponent_get_dtstart(event);
            QDateTime start = createDateTimeFromTimeType(dtstart);

            icaltimetype dtend = icalcomponent_get_dtend(event);
            QDateTime end = createDateTimeFromTimeType(dtend);

            // RRULE
            bool isRecurrent = false;
            icalproperty *prop = icalcomponent_get_first_property(event, ICAL_RRULE_PROPERTY);
            icalrecurrencetype rrule = {};
            if (prop) {
                isRecurrent = true;
                rrule = icalproperty_get_rrule(prop);
            }

            // RID: The first ever recorded time of a recurrent event instance. We'll use
            // 'UID-UNIX_TIMESTAMP' as ID.
            bool isUpdatedRecurrence = false;
            bool isCancelledRecurrence = false;
            icaltimetype rid = icalcomponent_get_recurrenceid(event);
            if (!icaltime_is_null_time(rid)) {
                if (exdatesById.value(id).contains(start)) {
                    isCancelledRecurrence = true;
                } else {
                    isUpdatedRecurrence = true;
                    id += QString("-%1").arg(createDateTimeFromTimeType(rid).toMSecsSinceEpoch());
                }
            }

            // Multi-day handling
            bool isMultiDay = start.daysTo(end.addSecs(-1)) > 0 && end > m_config.currentTime;
            if (isMultiDay && end > m_config.timeRangeEnd) {
                end = m_config.timeRangeEnd;
            }

            // Status filter
            icalproperty_status status = icalcomponent_get_status(event);
            bool isCancelled = (status == ICAL_STATUS_CANCELLED || status == ICAL_STATUS_FAILED
                                || status == ICAL_STATUS_DELETED || isCancelledRecurrence);

            // Skip cancelled or non-recurrent events that are outside of our date range
            if (isCancelled
                || (!isRecurrent && !isUpdatedRecurrence
                    && ((start < m_config.timeRangeStart && !isMultiDay)
                        || start >= m_config.timeRangeEnd || end < m_config.currentTime))) {
                continue;
            }

            QString summary = icalcomponent_get_summary(event);
            QString location = icalcomponent_get_location(event);
            QString description = icalcomponent_get_description(event);

            if (isRecurrent) { // Recurrent origin event, parsed first
                // Get EXDATE's
                icaltimetype exdate = {};
                QList<QDateTime> exdates;
                for (icalproperty *prop =
                             icalcomponent_get_first_property(event, ICAL_EXDATE_PROPERTY);
                     prop != nullptr;
                     prop = icalcomponent_get_next_property(event, ICAL_EXDATE_PROPERTY)) {
                    exdate = icalproperty_get_exdate(prop);
                    exdates.append(createDateTimeFromTimeType(exdate));
                }
                exdatesById[id] = exdates;

                icalrecur_iterator *recurrenceIter = icalrecur_iterator_new(rrule, dtstart);
                if (recurrenceIter) {
                    // INFO: Since libical v3.0, a start time limit can be specified for recurrence
                    // iterators in order to reduce parsing overhead, i.e. for old events that are
                    // irrelevant to us. This only works for RRULE's that do not contain COUNT.
                    // https://github.com/libical/libical/blob/3.0/src/libical/icalrecur.h#L291
                    if (rrule.count == 0) {
                        QDateTime timeRangeStart = m_config.timeRangeStart.toUTC();

                        struct icaltimetype recurStartCap = icaltime_null_time();
                        recurStartCap.year = timeRangeStart.date().year();
                        recurStartCap.month = timeRangeStart.date().month();
                        recurStartCap.day = timeRangeStart.date().day();
                        recurStartCap.hour = timeRangeStart.time().hour();
                        recurStartCap.minute = timeRangeStart.time().minute();
                        recurStartCap.second = timeRangeStart.time().second();
                        recurStartCap.is_date = false;

                        if (icaltime_is_valid_time(recurStartCap)) {
                            if (!icalrecur_iterator_set_start(recurrenceIter, recurStartCap)) {
                                qCCritical(lcCalDAVEventFeeder)
                                        << "Failed to set RRULE iterator starting date:"
                                        << icalerror_strerror(icalerrno);

                                icalrecur_iterator_free(recurrenceIter);

                                return false;
                            }
                        } else {
                            qCDebug(lcCalDAVEventFeeder)
                                    << "Invalid RRULE iterator starting date - skipping:"
                                    << icalerror_strerror(icalerrno);
                        }
                    }

                    qint64 duration = start.secsTo(end);

                    for (icaltimetype next = icalrecur_iterator_next(recurrenceIter);
                         !icaltime_is_null_time(next);
                         next = icalrecur_iterator_next(recurrenceIter)) {
                        QDateTime recurStart = createDateTimeFromTimeType(next);
                        QDateTime recurEnd = recurStart.addSecs(duration);
                        if (recurStart >= m_config.timeRangeEnd) {
                            break;
                        } else if (recurEnd < m_config.currentTime) {
                            continue;
                        }

                        // Recurrent multi-day handling
                        bool recurMultiDay = recurStart.daysTo(recurEnd.addSecs(-1)) > 0
                                && recurEnd > m_config.currentTime;
                        if (recurMultiDay && recurEnd > m_config.timeRangeEnd) {
                            recurEnd = m_config.timeRangeEnd;
                        }

                        if (!exdates.contains(recurStart)
                            && (recurStart >= m_config.timeRangeStart || recurMultiDay)) {
                            QString recurId =
                                    QString("%1-%2").arg(id).arg(recurStart.toMSecsSinceEpoch());
                            manager.addDateEvent(recurId, source, recurStart, recurEnd, summary,
                                                 location, description);
                        }
                    }

                    icalrecur_iterator_free(recurrenceIter);
                }
            } else if (isUpdatedRecurrence) { // Updates of a recurrent event instance
                if ((start < m_config.timeRangeStart && !isMultiDay)
                    || start >= m_config.timeRangeEnd || end < m_config.currentTime) {
                    // Updated recurrence doesn't match our criteria anymore
                    manager.removeDateEvent(id, start, end);
                } else if (manager.isAddedDateEvent(id)) {
                    // Exists but modified
                    manager.modifyDateEvent(id, source, start, end, summary, location, description);
                } else {
                    // Does not exist, e.g. moved from past to future, different day
                    manager.addDateEvent(id, source, start, end, summary, location, description);
                }
            } else { // Normal event, no recurrence, or update of a recurrent instance
                manager.addDateEvent(id, source, start, end, summary, location, description);
            }
        }

        icalcomponent_free(calendar);
    }

    if (icalerrno != ICAL_NO_ERROR) {
        qCCritical(lcCalDAVEventFeeder) << "ical error:" << icalerror_strerror(icalerrno);
        return false;
    }
    return true;
}

bool CalDAVEventFeeder::responseDataChanged(const QByteArray &data)
{
    const QStringList items = QString(data).split('\n');

    // Filter MS entries that aren't used but change on a per-export basis
    QStringList cleaned;
    for (const QString &item : items) {
        if (!item.contains("X-MICROSOFT-CDO-")) {
            cleaned << item;
        }
    }

    QString result = cleaned.join('\n');
    size_t checksum = qHash(result);

    if (m_checksums.contains(checksum)) {
        return false;
    }

    m_checksums.append(checksum);
    return true;
}

QDateTime CalDAVEventFeeder::createDateTimeFromTimeType(icaltimetype &datetime)
{
    /*
        Datetime values that feature a non-local VTIMEZONE / TZID
        (e.g. TZID=America/New_York:19980119T020000, or TZID=UTC:) will have to be
        converted to local time.

        We'll check if we can extract a valid timezone, otherwise we'll
        treat it as local/floating.

        A floating 20220816T054500 simply applies to the current timezone.
        Events with users that are located in different timezones would
        hence take place "several times" on a given day.

        See: https://www.rfc-editor.org/rfc/rfc5545#section-3.3.5
    */
    icaltimezone *zone = const_cast<icaltimezone *>(datetime.zone);
    int offset = icaltimezone_get_utc_offset(zone, &datetime, &datetime.is_daylight);
    QTimeZone convertZone(offset);
    if (zone && convertZone.isValid()) {
        return QDateTime(QDate(datetime.year, datetime.month, datetime.day),
                         QTime(datetime.hour, datetime.minute, datetime.second), convertZone)
                .toLocalTime();
    } else {
        return QDateTime(QDate(datetime.year, datetime.month, datetime.day),
                         QTime(datetime.hour, datetime.minute, datetime.second));
    }
}
