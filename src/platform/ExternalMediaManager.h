#pragma once

#include <QObject>

class ExternalMediaManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ExternalMediaManager)

public:
    Q_REQUIRED_RESULT static ExternalMediaManager &instance();

    virtual void pause() = 0;
    virtual void resume() = 0;
    virtual bool hasState() const = 0;

    explicit ExternalMediaManager() : QObject() { }
    ~ExternalMediaManager() = default;
};
