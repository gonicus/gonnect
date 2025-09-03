#pragma once

#include <QObject>

class BackgroundManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(BackgroundManager)

public:
    Q_REQUIRED_RESULT static BackgroundManager &instance();

    explicit BackgroundManager() : QObject() { }
    ~BackgroundManager() = default;

    virtual void request(bool autostart) = 0;
    virtual bool autostart() = 0;

signals:
    void autostartChanged();
};
