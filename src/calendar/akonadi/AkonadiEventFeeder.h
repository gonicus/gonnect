#pragma once

#include <QObject>
#include <QDateTime>
#include <QRegularExpression>

#include <Akonadi/Session>
#include <Akonadi/Monitor>

#include <Akonadi/CalendarUtils>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>

#include <KCalendarCore/Event>
#include <KCalendarCore/Incidence>
#include <KCalendarCore/Recurrence>

#include "IDateEventFeeder.h"

class AkonadiEventFeeder : public QObject, public IDateEventFeeder
{
    Q_OBJECT

public:
    explicit AkonadiEventFeeder(QObject *parent = nullptr, const QString &source = "",
                                const QDateTime &timeRangeStart = QDateTime(),
                                const QDateTime &timeRangeEnd = QDateTime());

    ~AkonadiEventFeeder();

    virtual void init() override;
    virtual QUrl networkCheckURL() const override { return QUrl(); };

    void process();

private Q_SLOTS:
    void processCollections(KJob *job);

private:
    QString m_source;
    QDateTime m_timeRangeStart;
    QDateTime m_timeRangeEnd;

    Akonadi::Session *m_session = nullptr;
    Akonadi::Monitor *m_monitor = nullptr;
    Akonadi::CollectionFetchJob *m_job = nullptr;
};
