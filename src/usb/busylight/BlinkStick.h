#pragma once

#include "IBusylightDevice.h"
#include "hidapi.h"

class BlinkStick : public IBusylightDevice
{
    Q_OBJECT

public:
    BlinkStick(const hid_device_info &deviceInfo, QObject *parent = nullptr)
        : IBusylightDevice{ deviceInfo, parent } { };

    QSet<SupportedCommands> supportedCommands() override;

protected:
    void send(bool on) override;
};
