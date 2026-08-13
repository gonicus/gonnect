#include "BlinkStick.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcBlinkStick, "gonnect.usb.busylight.BlinkStick")

QSet<IBusylightDevice::SupportedCommands> BlinkStick::supportedCommands()
{
    static QSet<IBusylightDevice::SupportedCommands> _commands = {
        IBusylightDevice::SupportedCommands::BusylightOnOff,
        IBusylightDevice::SupportedCommands::BusylightColor,
    };
    return _commands;
}

void BlinkStick::send(bool on)
{
    if (!m_device) {
        qCCritical(lcBlinkStick) << "Error: trying to send data while the USB device is not open";
        return;
    }

    const int ledCount = 8; // BlinkStick Strip Mini has 8 LEDs
    const quint8 red = on ? static_cast<quint8>(m_color.red()) : 0x00;
    const quint8 green = on ? static_cast<quint8>(m_color.green()) : 0x00;
    const quint8 blue = on ? static_cast<quint8>(m_color.blue()) : 0x00;

    qCDebug(lcBlinkStick) << "Sending color to all" << ledCount << "LEDs: R=" << red
                          << "G=" << green << "B=" << blue;

    bool success = true;

    for (int i = 0; i < ledCount; ++i) {
        unsigned char buf[6];

        buf[0] = 0x05; // Report ID: indexed color
        buf[1] = 0x00; // Channel
        buf[2] = i; // LED index
        buf[3] = red;
        buf[4] = green;
        buf[5] = blue;

        if (hid_write(m_device, buf, sizeof(buf)) < 0) {
            qCWarning(lcBlinkStick)
                    << "failed to write to BlinkStick LED" << i << ":" << hid_error(m_device);
            success = false;
        }
    }

    if (success) {
        qCDebug(lcBlinkStick) << "Successfully wrote color to all" << ledCount << "BlinkStick LEDs";
    }
}
