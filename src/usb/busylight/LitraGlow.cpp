#include "LitraGlow.h"
#include <QLoggingCategory>

#define REPORT_ID 0x11
#define DEVICE_ID 0xFF
#define SOFTWARE_ID 0x0A
#define SET_ILLUMINATION 0x10
#define FEATURE_ILLUMINATION 0x04

Q_LOGGING_CATEGORY(lcLitraGlow, "gonnect.usb.busylight.LitraGlow")

QSet<IBusylightDevice::SupportedCommands> LitraGlow::supportedCommands()
{
    static QSet<IBusylightDevice::SupportedCommands> _commands = {
        IBusylightDevice::SupportedCommands::StreamlightOnOff,
    };
    return _commands;
}

void LitraGlow::switchStreamlight(bool on)
{
    if (!m_device) {
        qCCritical(lcLitraGlow) << "Error: trying to send data while the USB device is not open";
        return;
    }

    unsigned char buf[20];

    // TODO: Make a hid++ support library and do this in a non hardcoded way
    //       because index and feature versions are not guaranteed to stay this
    //       way.

    // Switch on or off
    memset(buf, 0, sizeof(buf));
    buf[0] = REPORT_ID;
    buf[1] = DEVICE_ID;
    buf[2] = FEATURE_ILLUMINATION;
    buf[3] = SET_ILLUMINATION | SOFTWARE_ID;
    buf[4] = on ? 1 : 0;

    hid_write(m_device, buf, sizeof(buf));
}

void LitraGlow::send(bool on)
{
    Q_UNUSED(on)
}
