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

    QSet<SupportedCommands> supportedCommands() override;
    void switchStreamlight(bool on) override;

private:
    bool hidppTransaction(unsigned char *buf, size_t len);

protected:
    void send(bool on) override;

    bool m_state = false;
    bool m_blinkState = false;
};
