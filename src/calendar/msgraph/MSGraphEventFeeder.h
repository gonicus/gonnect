#pragma once

#include <QObject>
#include <QDateTime>
#include <QNetworkReply>
#include <QTimer>

#include "IDateEventFeeder.h"

class QJsonValue;
class DateEventFeederManager;

class MSGraphEventFeeder : public QObject, public IDateEventFeeder
{
    Q_OBJECT

public:
    explicit MSGraphEventFeeder(const QString &source, const QDateTime &timeRangeStart,
                                const QDateTime &timeRangeEnd, const int retryCount,
                                const int retryInterval, DateEventFeederManager *parent = nullptr);
    ~MSGraphEventFeeder();

    void init() override;
    QUrl networkCheckURL() const override;

Q_SIGNALS:
    void feederFailed();

private Q_SLOTS:
    void requestEvents();

private:
    void refreshOrRequestLogin();
    void eventsReceived(QNetworkReply *reply);
    void errorOccurred(QNetworkReply *reply, QNetworkReply::NetworkError code);
    static QDateTime parseDateTime(const QJsonValue &dateTimeContainer);

    const QString m_source;
    const QDateTime m_timeRangeStart;
    const QDateTime m_timeRangeEnd;
    DateEventFeederManager *m_manager = nullptr;
    QNetworkAccessManager *m_networkAccessManager = nullptr;
    QTimer m_calendarRefreshTimer;
    bool m_isFirstPage = true;

    int m_retryCount = 0;
    int m_retryInterval = 0;
};
