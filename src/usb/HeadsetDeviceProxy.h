#pragma once
#include <hidapi.h>
#include <QObject>
#include <QTimer>
#include <QtQml/qqml.h>
#include <QtQml/qqmlregistration.h>
#include "IHeadsetDevice.h"
#include "AppSettings.h"

class HeadsetDevice;

class HeadsetDeviceProxy : public IHeadsetDevice
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)

public:
    explicit HeadsetDeviceProxy(QObject *parent = nullptr);

    virtual QString name() const;

    virtual bool open() final;
    virtual void close() final;

    virtual void setBusyLine(bool flag);
    virtual bool getBusyLine() const;

    virtual bool getHookSwitch() const;
    virtual bool getFlash() const;

    virtual void setRing(bool flag);
    virtual void setHold(bool flag);

    virtual void setMute(bool flag);
    virtual bool getMute() const;

    virtual void setIdle();

    bool available() const { return !!m_device; }

    ~HeadsetDeviceProxy();

signals:
    void nameChanged();

private:
    bool refreshDevice();
    bool isEnabled() { return m_settings.value("generic/useHeadset", false).toBool(); }

    AppSettings m_settings;
    HeadsetDevice *m_device = nullptr;
};

class HeadsetDeviceProxyWrapper
{
    Q_GADGET
    QML_FOREIGN(HeadsetDeviceProxy)
    QML_NAMED_ELEMENT(HeadsetDeviceProxy)
    QML_UNCREATABLE("object is owned by the main application")
};
