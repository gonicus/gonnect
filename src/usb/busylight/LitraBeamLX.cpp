#include "LitraBeamLX.h"
#include <QLoggingCategory>

#define REPORT_ID 0x11
#define DEVICE_ID 0xFF
#define SOFTWARE_ID 0x0A
#define SET_ILLUMINATION 0x40
#define SET_INDIVITUAL_RGB_ZONES 0x10
#define FRAME_END 0x70
#define FEATURE_BRIGHTNESS_CONTROL 0x0A
#define FEATURE_PREKEY_LIGHTING 0x0C

Q_LOGGING_CATEGORY(lcLitraBeamLX, "gonnect.usb.busylight.LitraBeamLX")

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
    buf[0] = REPORT_ID;
    buf[1] = DEVICE_ID;
    buf[2] = FEATURE_BRIGHTNESS_CONTROL;
    buf[3] = SET_ILLUMINATION | SOFTWARE_ID;
    buf[4] = on ? 1 : 0;

    hid_write(m_device, buf, sizeof(buf));

    if (on) {
        // rgb values
        memset(buf, 0, sizeof(buf));
        buf[0] = REPORT_ID;
        buf[1] = DEVICE_ID;
        buf[2] = FEATURE_PREKEY_LIGHTING;
        buf[3] = SET_INDIVITUAL_RGB_ZONES | SOFTWARE_ID;
        for (auto i = 0; i < 4; ++i) {
            buf[4 + i * 4] = i + 1; // Zone number
            buf[5 + i * 4] = m_color.red();
            buf[6 + i * 4] = m_color.green();
            buf[7 + i * 4] = m_color.blue();
        }

        hid_write(m_device, buf, sizeof(buf));

        memset(buf, 0, sizeof(buf));
        buf[0] = REPORT_ID;
        buf[1] = DEVICE_ID;
        buf[2] = FEATURE_PREKEY_LIGHTING;
        buf[3] = SET_INDIVITUAL_RGB_ZONES | SOFTWARE_ID;
        for (auto i = 0; i < 4; ++i) {
            buf[4 + i * 4] = i + 5;
            buf[5 + i * 4] = m_color.red();
            buf[6 + i * 4] = m_color.green();
            buf[7 + i * 4] = m_color.blue();
        }

        hid_write(m_device, buf, sizeof(buf));

        // Flush
        memset(buf, 0, sizeof(buf));
        buf[0] = REPORT_ID;
        buf[1] = DEVICE_ID;
        buf[2] = FEATURE_PREKEY_LIGHTING;
        buf[3] = FRAME_END | SOFTWARE_ID;
        buf[4] = 1; // Persist

        hid_write(m_device, buf, sizeof(buf));
    }
}
