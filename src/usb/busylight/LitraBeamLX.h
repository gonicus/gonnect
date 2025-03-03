#pragma once

#include <QTimer>

#include "IBusylightDevice.h"
#include "hidapi.h"

class LitraBeamLX : public IBusylightDevice
{
    Q_OBJECT

public:
    LitraBeamLX(const hid_device_info &deviceInfo, QObject *parent = nullptr)
        : IBusylightDevice{ deviceInfo, parent } { };

    virtual QSet<SupportedCommands> supportedCommands() override;
    virtual void switchStreamlight(bool on) override;

protected:
    virtual void send(bool on) override;
};
