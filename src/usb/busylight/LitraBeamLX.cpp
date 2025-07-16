#include "LitraBeamLX.h"
#include <QLoggingCategory>

#define LX_REPORT_ID 0x11
#define LX_DEVICE_ID 0xFF
#define LX_SOFTWARE_ID 0x0A
#define LX_SET_ILLUMINATION 0x10
#define LX_SET_ILLUMINATION_RGB 0x40
#define LX_SET_INDIVITUAL_RGB_ZONES 0x10
#define LX_FRAME_END 0x70
#define LX_FEATURE_ILLUMINATION 0x06
#define LX_FEATURE_BRIGHTNESS_CONTROL 0x0A
#define LX_FEATURE_PREKEY_LIGHTING 0x0C

Q_LOGGING_CATEGORY(lcLitraBeamLX, "gonnect.usb.busylight.LitraBeamLX")

QSet<IBusylightDevice::SupportedCommands> LitraBeamLX::supportedCommands()
{
    static QSet<IBusylightDevice::SupportedCommands> _commands = {
        IBusylightDevice::SupportedCommands::BusylightOnOff,
        IBusylightDevice::SupportedCommands::BusylightColor,
        IBusylightDevice::SupportedCommands::StreamlightOnOff,
    };
    return _commands;
}

void LitraBeamLX::switchStreamlight(bool on)
{
    if (!m_device) {
        qCCritical(lcLitraBeamLX) << "Error: trying to send data while the USB device is not open";
        return;
    }

    unsigned char buf[20];

    // TODO: Make a hid++ support library and do this in a non hardcoded way
    //       because index and feature versions are not guaranteed to stay this
    //       way.

    // Switch on or off
    memset(buf, 0, sizeof(buf));
    buf[0] = LX_REPORT_ID;
    buf[1] = LX_DEVICE_ID;
    buf[2] = LX_FEATURE_ILLUMINATION;
    buf[3] = LX_SET_ILLUMINATION | LX_SOFTWARE_ID;
    buf[4] = on ? 1 : 0;

    hid_write(m_device, buf, sizeof(buf));
}

void LitraBeamLX::send(bool on)
{
    if (!m_device) {
        qCCritical(lcLitraBeamLX) << "Error: trying to send data while the USB device is not open";
        return;
    }

    unsigned char buf[20];

    // TODO: Make a hid++ support library and do this in a non hardcoded way
    //       because index and feature versions are not guaranteed to stay this
    //       way.

    // Switch on or off
    memset(buf, 0, sizeof(buf));
    buf[0] = LX_REPORT_ID;
    buf[1] = LX_DEVICE_ID;
    buf[2] = LX_FEATURE_BRIGHTNESS_CONTROL;
    buf[3] = LX_SET_ILLUMINATION_RGB | LX_SOFTWARE_ID;
    buf[4] = on ? 1 : 0;

    hid_write(m_device, buf, sizeof(buf));

    hid_read_timeout(m_device, buf, 20, 1000);

    if (on) {
        // rgb values
        memset(buf, 0, sizeof(buf));
        buf[0] = LX_REPORT_ID;
        buf[1] = LX_DEVICE_ID;
        buf[2] = LX_FEATURE_PREKEY_LIGHTING;
        buf[3] = LX_SET_INDIVITUAL_RGB_ZONES | LX_SOFTWARE_ID;
        for (auto i = 0; i < 4; ++i) {
            buf[4 + i * 4] = i + 1; // Zone number
            buf[5 + i * 4] = m_color.red();
            buf[6 + i * 4] = m_color.green();
            buf[7 + i * 4] = m_color.blue();
        }

        hid_write(m_device, buf, sizeof(buf));
        hid_read_timeout(m_device, buf, 20, 1000);

        memset(buf, 0, sizeof(buf));
        buf[0] = LX_REPORT_ID;
        buf[1] = LX_DEVICE_ID;
        buf[2] = LX_FEATURE_PREKEY_LIGHTING;
        buf[3] = LX_SET_INDIVITUAL_RGB_ZONES | LX_SOFTWARE_ID;
        for (auto i = 0; i < 4; ++i) {
            buf[4 + i * 4] = i + 5;
            buf[5 + i * 4] = m_color.red();
            buf[6 + i * 4] = m_color.green();
            buf[7 + i * 4] = m_color.blue();
        }

        hid_write(m_device, buf, sizeof(buf));
        hid_read_timeout(m_device, buf, 20, 1000);

        // Flush
        memset(buf, 0, sizeof(buf));
        buf[0] = LX_REPORT_ID;
        buf[1] = LX_DEVICE_ID;
        buf[2] = LX_FEATURE_PREKEY_LIGHTING;
        buf[3] = LX_FRAME_END | LX_SOFTWARE_ID;
        buf[4] = 1; // Persist

        hid_write(m_device, buf, sizeof(buf));
        hid_read_timeout(m_device, buf, 20, 1000);
    }
}
