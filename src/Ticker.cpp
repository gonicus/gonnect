#include "Ticker.h"

#include <QDateTime>

Ticker::Ticker(QObject *parent) : QObject{ parent }
{
    m_dayTicker.setSingleShot(true);
    m_dayTicker.callOnTimeout(this, [this]() {
        Q_EMIT newDay();
        restartDailyTimer();
    });
    restartDailyTimer();
}

void Ticker::restartDailyTimer()
{
    const auto now = QDateTime::currentDateTime();
    const QDateTime next(now.addDays(1).date(), QTime(1, 0));
    const qint64 ms = now.msecsTo(next);
    m_dayTicker.start(ms);
}
