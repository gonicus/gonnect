#pragma once

#include <QTimer>

/// Singleton class that fires signals in regular intervals (e.g. once every new day).
class Ticker : public QObject
{
    Q_OBJECT

public:
    static Ticker &instance()
    {
        static Ticker *_instance = nullptr;
        if (!_instance) {
            _instance = new Ticker;
        }
        return *_instance;
    }

private Q_SLOTS:
    void restartDailyTimer();

private:
    explicit Ticker(QObject *parent = nullptr);

    QTimer m_dayTicker;

Q_SIGNALS:
    void newDay();
};
