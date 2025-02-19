#include "LuxaforFlag.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcLuxaforFlag, "gonnect.usb.busylight.LuxaforFlag")

void LuxaforFlag::send(bool on)
{
    if (!m_device) {
        qCCritical(lcLuxaforFlag) << "Error: trying to send data while the USB device is not open";
        return;
    }

    unsigned char buf[8];

    buf[0] = 0x01; // Command: Color
    buf[1] = 0xFF; // LED selection: All
    buf[2] = on ? m_color.red() : 0x00; // Red value
    buf[3] = on ? m_color.green() : 0x00; // Green value
    buf[4] = on ? m_color.blue() : 0x00; // Blue value
    buf[5] = 0x00; // Pad
    buf[6] = 0x00; // Pad
    buf[7] = 0x00; // Pad

    hid_write(m_device, buf, sizeof(buf));
}
