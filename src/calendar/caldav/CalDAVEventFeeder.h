#pragma once

#include <QObject>
#include <QDateTime>
#include <QTime>
#include <QTimer>

#include "IDateEventFeeder.h"
#include "CalDAVEventFeederConfig.h"
#include <libical/ical.h>
#include <libical/icalerror.h>
#include <QtWebDAV/qwebdav.h>
#include <QtWebDAV/qwebdavdirparser.h>

class CalDAVEventFeeder : public QObject, public IDateEventFeeder
{
    Q_OBJECT

public:
    explicit CalDAVEventFeeder(QObject *parent = nullptr,
                               const CalDAVEventFeederConfig &config = {});

    ~CalDAVEventFeeder();

    virtual void init() override;
    virtual QUrl networkCheckURL() const override;

    void process();

private Q_SLOTS:
    void onError(QString error) const;
    void onParserFinished();

private:
    void processResponse(const QByteArray &data);

    bool responseDataChanged(const QByteArray &data);
    QDateTime createDateTimeFromTimeType(const icaltimetype &datetime);

    QList<size_t> m_checksums;

    CalDAVEventFeederConfig m_config;

    QTimer m_calendarRefreshTimer;

    QWebdav m_webdav;
    QWebdavDirParser m_webdavParser;
};
