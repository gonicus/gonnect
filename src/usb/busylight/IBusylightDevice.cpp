#include "IBusylightDevice.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcBusylightDevice, "gonnect.usb.busylight.IBusylightDevice")

IBusylightDevice::IBusylightDevice(const hid_device_info &deviceInfo, QObject *parent)
    : QObject{ parent }, m_color(Qt::GlobalColor::red)
{
    m_path = deviceInfo.path;

    m_blinkTimer.setInterval(750);
    m_blinkTimer.callOnTimeout(this, [this]() {
        m_isOn = !m_isOn;
        send(m_isOn);
    });
}

IBusylightDevice::~IBusylightDevice()
{
    close();
}

bool IBusylightDevice::open()
{
    if (m_device) {
        return true;
    }

    hid_device *device = hid_open_path(m_path.toStdString().c_str());
    if (device) {
        m_device = device;
        switchOff();
    } else {
        qCCritical(lcBusylightDevice) << "Error: cannot open Luxafor Flag busylight device";
    }

    return device;
}

void IBusylightDevice::close()
{
    if (m_device) {
        hid_close(m_device);
        m_device = nullptr;
    }
}

void IBusylightDevice::switchOn(QColor color)
{
    m_color = color;
    stopBlinking();
    send(true);
    m_isOn = true;
}

void IBusylightDevice::switchOff()
{
    stopBlinking();
    send(false);
    m_isOn = false;
}

void IBusylightDevice::startBlinking(QColor color)
{
    m_color = color;
    m_blinkTimer.start();
}

void IBusylightDevice::stopBlinking()
{
    if (m_blinkTimer.isActive()) {
        m_blinkTimer.stop();
        switchOff();
    }
}
