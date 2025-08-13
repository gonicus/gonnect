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
    explicit CalDAVEventFeeder(QObject *parent = nullptr);
    ~CalDAVEventFeeder();

    void init(const QString &settingsGroupId, const QString &source, const QString &host,
              const QString &path, const QString &user, int port, bool useSSL, int interval,
              const QDateTime &timeRangeStart, const QDateTime &timeRangeEnd);

    virtual void process() override;
    virtual QUrl networkCheckURL() const override { return m_url; };

private slots:
    void onError(QString error) const;
    void onParserFinished();

private:
    void processResponse(const QByteArray &data);

    bool responseDataChanged(const QByteArray &data);
    QDateTime createDateTimeFromTimeType(const icaltimetype &datetime);

    QList<size_t> m_checksums;

    QString m_source;
    QDateTime m_timeRangeStart;
    QDateTime m_timeRangeEnd;

    QUrl m_url;
    QString m_settingsGroupId;
    QString m_host;
    QString m_path;
    QString m_user;
    int m_port = 0;
    bool m_useSSL = true;
    int m_interval = 300000;

    QTimer m_calendarRefreshTimer;

    QWebdav m_webdav;
    QWebdavDirParser m_webdavParser;
};
