#pragma once

#include <QTimer>

#include "IBusylightDevice.h"
#include "hidapi.h"

class LuxaforFlag : public IBusylightDevice
{
    Q_OBJECT

public:
    LuxaforFlag(const hid_device_info &deviceInfo, QObject *parent = nullptr)
        : IBusylightDevice{ deviceInfo, parent } { };

    QSet<SupportedCommands> supportedCommands() override;

protected:
    void send(bool on) override;
};
