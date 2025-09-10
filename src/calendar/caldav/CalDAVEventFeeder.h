#pragma once

#include <QObject>
#include <QDateTime>
#include <QTime>
#include <QTimer>

#include "IDateEventFeeder.h"
#include <libical/ical.h>
#include <libical/icalerror.h>
#include <QtWebDAV/qwebdav.h>
#include <QtWebDAV/qwebdavdirparser.h>

class CalDAVEventFeeder : public QObject, public IDateEventFeeder
{
    Q_OBJECT

public:
    explicit CalDAVEventFeeder(QObject *parent = nullptr, const QString &settingsGroupId = "",
                               const QString &source = "", const QString &host = "",
                               const QString &path = "", const QString &user = "", int port = 0,
                               bool useSSL = true, int interval = 300000,
                               const QDateTime &timeRangeStart = QDateTime(),
                               const QDateTime &timeRangeEnd = QDateTime());

    ~CalDAVEventFeeder();

    virtual void init() override;
    virtual QUrl networkCheckURL() const override { return m_url; };

    void process();

private Q_SLOTS:
    void onError(QString error) const;
    void onParserFinished();

private:
    void processResponse(const QByteArray &data);

    bool responseDataChanged(const QByteArray &data);
    QDateTime createDateTimeFromTimeType(const icaltimetype &datetime);

    QList<size_t> m_checksums;

    QString m_settingsGroupId;
    QString m_source;
    QString m_host;
    QString m_path;
    QString m_user;
    int m_port;
    bool m_useSSL;
    int m_interval;
    QDateTime m_timeRangeStart;
    QDateTime m_timeRangeEnd;
    QUrl m_url;

    QTimer m_calendarRefreshTimer;

    QWebdav m_webdav;
    QWebdavDirParser m_webdavParser;
};
