#pragma once
#include <QObject>

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

    virtual void setMute(bool flag) = 0;
    virtual bool getMute() const = 0;

    virtual void setIdle() = 0;

    ~IHeadsetDevice() = default;

signals:
    void hookSwitch();
    void mute();
    void busyLine();
    void flash();
};
