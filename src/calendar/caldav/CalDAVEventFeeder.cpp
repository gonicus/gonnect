#include <QMap>
#include <QLoggingCategory>
#include <QRegularExpression>

#include "CalDAVEventFeeder.h"
#include "DateEventFeederManager.h"
#include "DateEventManager.h"

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
    connect(&m_webdavParser, &QWebdavDirParser::finished, this,
            &CalDAVEventFeeder::onParserFinished);
    connect(&m_webdavParser, &QWebdavDirParser::errorChanged, this, &CalDAVEventFeeder::onError);
    connect(&m_webdav, &QWebdav::errorChanged, this, &CalDAVEventFeeder::onError);

    /*
        As the Kopano CalDAV server doesn't provide 'getetag' and 'getlastmodified' values,
        it's impossible to actively poll for changes. Thus, we're simply re-launching the
        process() loop, at least until there's a better solution.

        Kopano saves every VEVENT wrapped in a VCALENDAR entry in separate '.ics' files in
        '/caldav/<USER>/Kalender'. A full calendar is generated on the fly once requested.
    */
    m_calendarRefreshTimer.setInterval(m_config.interval);
    connect(&m_calendarRefreshTimer, &QTimer::timeout, this, [this]() { process(); });
    m_calendarRefreshTimer.start();

    process();
}

void CalDAVEventFeeder::onError(QString error) const
{
    qCCritical(lcCalDAVEventFeeder) << error;
}

void CalDAVEventFeeder::onParserFinished()
{
    const auto list = m_webdavParser.getList();
    for (const auto &item : list) {
        QNetworkReply *reply = m_webdav.get(item.path());
        connect(
                reply, &QNetworkReply::finished, this,
                [reply, this]() {
                    if (!reply) {
                        return;
                    }
                    QByteArray data = reply->readAll();
                    reply->deleteLater();

                    QMimeDatabase db;
                    QMimeType type = db.mimeTypeForData(data);
                    if (type.name() == "text/calendar" && !data.isEmpty()
                        && responseDataChanged(data)) {
                        DateEventManager &manager = DateEventManager::instance();
                        manager.removeDateEventsBySource(m_config.source);

                        processResponse(data);
                    }
                },
                Qt::ConnectionType::SingleShotConnection);
    }
}

void CalDAVEventFeeder::process()
{
    auto manager = q_check_ptr(qobject_cast<DateEventFeederManager *>(parent()));
    manager->acquireSecret(m_config.settingsGroupId, [this](const QString &password) {
        m_webdav.setConnectionSettings(m_config.useSSL ? QWebdav::HTTPS : QWebdav::HTTP,
                                       m_config.host, m_config.path, m_config.user, password,
                                       m_config.port);

        m_webdavParser.listDirectory(&m_webdav, "/");
    });
}

void CalDAVEventFeeder::processResponse(const QByteArray &data)
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
            if (isCancelled || (!isRecurrent && !isUpdatedRecurrence
                && ((start < m_config.timeRangeStart && !isMultiDay)
                    || start > m_config.timeRangeEnd || end < m_config.currentTime))) {
                continue;
            }

            QString summary = icalcomponent_get_summary(event);
            QString location = icalcomponent_get_location(event);
            QString description = icalcomponent_get_description(event);

            // Get EXDATE's
            icaltimetype exdate = {};
            QList<QDateTime> exdates;
            for (icalproperty *prop = icalcomponent_get_first_property(event, ICAL_EXDATE_PROPERTY);
                 prop != nullptr;
                 prop = icalcomponent_get_next_property(event, ICAL_EXDATE_PROPERTY)) {
                exdate = icalproperty_get_exdate(prop);
                exdates.append(createDateTimeFromTimeType(exdate));
            }
            exdatesById[id] = exdates;

            if (isRecurrent) {
                // Recurrent origin event, parsed first
                icalrecur_iterator *recurrenceIter = icalrecur_iterator_new(rrule, dtstart);

                if (recurrenceIter) {
                    qint64 duration = start.secsTo(end);

                    for (icaltimetype next = icalrecur_iterator_next(recurrenceIter);
                         !icaltime_is_null_time(next);
                         next = icalrecur_iterator_next(recurrenceIter)) {
                        QDateTime recurStart = createDateTimeFromTimeType(next);
                        QDateTime recurEnd = recurStart.addSecs(duration);
                        if (recurStart > m_config.timeRangeEnd) {
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
                            manager.addDateEvent(recurId, m_config.source, recurStart, recurEnd,
                                                 summary, location, description);
                        }
                    }

                    icalrecur_iterator_free(recurrenceIter);
                }
            } else if (isUpdatedRecurrence) {
                // Updates of a recurrent event instance
                if ((start < m_config.timeRangeStart && !isMultiDay)
                    || start > m_config.timeRangeEnd || end < m_config.currentTime) {
                    // Updated recurrence doesn't match our criteria anymore
                    manager.removeDateEvent(id, start, end);
                } else if (manager.isAddedDateEvent(id)) {
                    // Exists but modified
                    manager.modifyDateEvent(id, m_config.source, start, end, summary, location,
                                            description);
                } else {
                    // Does not exist, e.g. moved from past to future, different day
                    manager.addDateEvent(id, m_config.source, start, end, summary, location,
                                         description);
                }
            } else {
                // Normal event, no recurrence, or update of a recurrent instance
                manager.addDateEvent(id, m_config.source, start, end, summary, location,
                                     description);
            }
        }
    } else {
        qCCritical(lcCalDAVEventFeeder) << icalerror_strerror(icalerrno);
    }
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

QDateTime CalDAVEventFeeder::createDateTimeFromTimeType(const icaltimetype &datetime)
{
    QString zone = icaltimezone_get_tzid(const_cast<icaltimezone *>(datetime.zone));
    if (zone == "UTC") {
        /*
            Qt expects an ISO 8601 format with '-' and '.' delimiters, iCal omits them.

            This only needs to be done for UTC datetime (19980119T070000Z) values, as datetime
            values that include a TZID (TZID=America/New_York:19980119T020000) will already
            report the correct time.

            This doesn't have to be done on "floating" datetimes as well, because "20220816T054500"
            simply applies to the current timezone. Events with participants that are located in
            different timezones would hence take place "several times" on a given day.

            See: https://www.rfc-editor.org/rfc/rfc5545#section-3.3.5
        */
        return QDateTime::fromString(QString("%1-%2-%3T%4:%5:%6Z")
                                             .arg(datetime.year, 4, 10, '0')
                                             .arg(datetime.month, 2, 10, '0')
                                             .arg(datetime.day, 2, 10, '0')
                                             .arg(datetime.hour, 2, 10, '0')
                                             .arg(datetime.minute, 2, 10, '0')
                                             .arg(datetime.second, 2, 10, '0'),
                                     Qt::ISODate)
                .toLocalTime();
    } else {
        return QDateTime(QDate(datetime.year, datetime.month, datetime.day),
                         QTime(datetime.hour, datetime.minute, datetime.second));
    }
}
