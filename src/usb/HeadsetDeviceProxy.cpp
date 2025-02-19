#include <QDebug>
#include <QLoggingCategory>
#include "HeadsetDevices.h"
#include "HeadsetDevice.h"
#include "HeadsetDeviceProxy.h"

Q_LOGGING_CATEGORY(lcHeadsetProxy, "gonnect.usb.headsetproxy")

using namespace std::chrono_literals;

HeadsetDeviceProxy::HeadsetDeviceProxy(QObject *parent) : IHeadsetDevice(parent)
{
    auto &devs = HeadsetDevices::instance();
    connect(&devs, &HeadsetDevices::devicesChanged, this, &HeadsetDeviceProxy::refreshDevice);

    refreshDevice();
    open();
}

HeadsetDeviceProxy::~HeadsetDeviceProxy()
{
    close();
}

bool HeadsetDeviceProxy::refreshDevice()
{
    auto devs = HeadsetDevices::instance().headsetDevices();

    if (m_device) {
        m_device = nullptr;
    }

    if (devs.count()) {
        m_device = devs.first();

        connect(m_device, &HeadsetDevice::hookSwitch, this, [this]() {
            if (isEnabled()) {
                emit hookSwitch();
            }
        });
        connect(m_device, &HeadsetDevice::mute, this, [this]() {
            if (isEnabled()) {
                emit mute();
            }
        });
        connect(m_device, &HeadsetDevice::busyLine, this, [this]() {
            if (isEnabled()) {
                emit busyLine();
            }
        });
        connect(m_device, &HeadsetDevice::flash, this, [this]() {
            if (isEnabled()) {
                emit flash();
            }
        });

        open();
    }

    emit nameChanged();

    return !!m_device;
}

QString HeadsetDeviceProxy::name() const
{
    if (m_device) {
        return m_device->name();
    }

    return "";
}

bool HeadsetDeviceProxy::getBusyLine() const
{
    if (m_device) {
        return m_device->getBusyLine();
    }

    return false;
}

bool HeadsetDeviceProxy::getHookSwitch() const
{
    if (m_device) {
        return m_device->getHookSwitch();
    }

    return false;
}

bool HeadsetDeviceProxy::getFlash() const
{
    if (m_device) {
        return m_device->getFlash();
    }

    return false;
}

bool HeadsetDeviceProxy::getMute() const
{
    if (m_device) {
        return m_device->getMute();
    }

    return false;
}

bool HeadsetDeviceProxy::open()
{
    if (m_device && !m_device->isOpen()) {
        if (m_device->open()) {
            qCInfo(lcHeadsetProxy) << "Using USB headset:" << HeadsetDeviceProxy::name();
            m_device->setIdle();
            return true;
        }
    }

    return false;
}

void HeadsetDeviceProxy::close()
{
    if (m_device) {
        m_device->close();
    }
}

void HeadsetDeviceProxy::setIdle()
{
    if (m_device) {
        m_device->setIdle();
    }
}

void HeadsetDeviceProxy::setBusyLine(bool flag)
{
    if (isEnabled() && m_device) {
        m_device->setBusyLine(flag);
    }
}

void HeadsetDeviceProxy::setMute(bool flag)
{
    if (isEnabled() && m_device) {
        m_device->setMute(flag);
    }
}

void HeadsetDeviceProxy::setRing(bool flag)
{
    if (isEnabled() && m_device) {
        m_device->setRing(flag);
    }
}

void HeadsetDeviceProxy::setHold(bool flag)
{
    if (isEnabled() && m_device) {
        m_device->setHold(flag);
    }
}
