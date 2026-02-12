#pragma once

#include <QTimer>

#include "IBusylightDevice.h"
#include "hidapi.h"

class LitraGlow : public IBusylightDevice
{
    Q_OBJECT

public:
    LitraGlow(const hid_device_info &deviceInfo, QObject *parent = nullptr)
        : IBusylightDevice{ deviceInfo, parent } { };

    QSet<SupportedCommands> supportedCommands() override;
    void switchStreamlight(bool on) override;

protected:
    void send(bool on) override;
};
