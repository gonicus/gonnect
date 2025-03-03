#include "KuandoOmega.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcKuandoOmega, "gonnect.usb.busylight.KuandoOmega")

using namespace std::chrono_literals;

KuandoOmega::KuandoOmega(const hid_device_info &deviceInfo, QObject *parent)
    : IBusylightDevice{ deviceInfo, parent }
{
    m_keepAliveTimer.setInterval(10s);
    m_keepAliveTimer.callOnTimeout(this, &KuandoOmega::sendKeepAlive);
}

QSet<IBusylightDevice::SupportedCommands> KuandoOmega::supportedCommands()
{
    static QSet<IBusylightDevice::SupportedCommands> _commands = {
        IBusylightDevice::SupportedCommands::BusylightOnOff,
        IBusylightDevice::SupportedCommands::BusylightColor,
    };
    return _commands;
}

void KuandoOmega::send(bool on)
{
    if (!on) {
        m_keepAliveTimer.stop();
    }

    if (!m_device) {
        qCCritical(lcKuandoOmega) << "Error: trying to send data while the USB device is not open";
        return;
    }

    if (on) {
        m_keepAliveTimer.start();
    }

    unsigned char buf[64] = { 0 };

    // Padding
    buf[59] = 0xFF;
    buf[60] = 0xFF;
    buf[61] = 0xFF;

    // Jump step 1
    buf[0] = 0x10; // Command: Jump, target: 0
    buf[1] = 0x00; // Repeat
    buf[2] = on ? m_color.red() : 0x00; // Red value
    buf[3] = on ? m_color.green() : 0x00; // Green value
    buf[4] = on ? m_color.blue() : 0x00; // Blue value

    // Checksum
    quint16 checksum = 0;
    for (int i = 0; i < 62; ++i) {
        checksum += buf[i];
    }

    buf[62] = checksum >> 8;
    buf[63] = checksum & 0xFF;

    hid_write(m_device, buf, sizeof(buf));
}

void KuandoOmega::sendKeepAlive() const
{
    if (!m_device) {
        qCCritical(lcKuandoOmega) << "Error: trying to send data while the USB device is not open";
        return;
    }

    unsigned char buf[64] = { 0 };

    // Padding
    buf[59] = 0xFF;
    buf[60] = 0xFF;
    buf[61] = 0xFF;

    // Keppalive step 1
    buf[0] = 0x8F; // Command: Keepalive, timeout: 15 sec

    // Checksum
    quint16 checksum = 0;
    for (int i = 0; i < 62; ++i) {
        checksum += buf[i];
    }

    buf[62] = checksum >> 8;
    buf[63] = checksum & 0xFF;

    hid_write(m_device, buf, sizeof(buf));
}
