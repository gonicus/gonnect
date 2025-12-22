#pragma once

#include <QObject>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(lcInhibit)

class InhibitHelper : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(InhibitHelper)

public:
    Q_REQUIRED_RESULT static InhibitHelper &instance();

    explicit InhibitHelper(QObject *parent = nullptr) : QObject(parent) { }
    ~InhibitHelper() = default;

    enum InhibitState { RUNNING = 1, QUERY_END, ENDING };
    Q_ENUM(InhibitState)

    enum InhibitFlag { LOGOUT = 1, USER_SWITCH = 2, SUSPEND = 4, IDLE = 8 };
    Q_ENUM(InhibitFlag)

    virtual void inhibit(unsigned int flags, const QString &reason) = 0;
    virtual void release() = 0;

    virtual void inhibitScreenSaver(const QString &applicationName, const QString &reason) = 0;
    virtual void releaseScreenSaver() = 0;

Q_SIGNALS:
    void stateChanged(bool screensaverActive, InhibitHelper::InhibitState state);
};
