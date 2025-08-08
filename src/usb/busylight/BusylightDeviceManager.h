#pragma once

#include <QObject>
#include <QColor>
#include "hidapi.h"

class IBusylightDevice;

class BusylightDeviceManager : public QObject
{
    Q_OBJECT

public:
    static BusylightDeviceManager &instance()
    {
        static BusylightDeviceManager *_instance = nullptr;
        if (!_instance) {
            _instance = new BusylightDeviceManager;
        }
        return *_instance;
    }

    bool createBusylightDevice(const struct hid_device_info &deviceInfo);
    void clearDevices();

    void switchOn(QColor color) const;
    void switchOff() const;

    void startBlinking(QColor color) const;
    void stopBlinking() const;

    void switchStreamlightOn() const;
    void switchStreamlightOff() const;

private slots:
    void updateBusylightState();

private:
    explicit BusylightDeviceManager(QObject *parent = nullptr);

    QList<IBusylightDevice *> m_devices;
};
