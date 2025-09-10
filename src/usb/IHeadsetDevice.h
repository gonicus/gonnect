#pragma once
#include <QObject>
#include "ReportDescriptorEnums.h"

class IHeadsetDevice : public QObject
{
    Q_OBJECT

public:
    IHeadsetDevice(QObject *parent = nullptr) : QObject(parent) { }

    virtual QString name() const = 0;

    virtual bool open() = 0;
    virtual void close() = 0;

    virtual void setBusyLine(bool flag) = 0;
    virtual bool getBusyLine() const = 0;

    virtual bool getHookSwitch() const = 0;
    virtual bool getFlash() const = 0;

    virtual void setRing(bool flag) = 0;
    virtual void setHold(bool flag) = 0;

    virtual void setIdle() = 0;

    virtual void syncDateAndTime() = 0;
    virtual void setLocalUserName(const QString &name) = 0;
    virtual void setLocalUserNumber(const QString &number) = 0;
    virtual void setLocalUserStatus(const QString &status) = 0;
    virtual void setOtherUserName(const QString &name) = 0;
    virtual void setOtherUserNumber(const QString &number) = 0;
    virtual void setSubject(const QString &subject) = 0;
    virtual void selectScreen(ReportDescriptorEnums::TeamsScreenSelect screen, bool clear = false,
                              bool backlight = true) = 0;
    virtual void setPresenceIcon(ReportDescriptorEnums::TeamsPresenceIcon) = 0;
    virtual void setCallStatus(const QString &state) = 0;

    ~IHeadsetDevice() = default;

Q_SIGNALS:
    void hookSwitch();
    void mute();
    void busyLine();
    void flash();
    void programmableButton();
    void teamsButton();
    void dial(const QString &number);
};
