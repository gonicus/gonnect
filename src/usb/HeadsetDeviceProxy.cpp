#include <QDebug>
#include <QLoggingCategory>
#include "UserInfo.h"
#include "USBDevices.h"
#include "HeadsetDevice.h"
#include "HeadsetDeviceProxy.h"

Q_LOGGING_CATEGORY(lcHeadsetProxy, "gonnect.usb.headsetproxy")

using namespace std::chrono_literals;

HeadsetDeviceProxy::HeadsetDeviceProxy(QObject *parent) : IHeadsetDevice(parent)
{
    auto &devs = USBDevices::instance();
    connect(&devs, &USBDevices::devicesChanged, this, &HeadsetDeviceProxy::refreshDevice);

    refreshDevice();
    open();
}

HeadsetDeviceProxy::~HeadsetDeviceProxy()
{
    close();
}

bool HeadsetDeviceProxy::refreshDevice()
{
    auto devs = USBDevices::instance().headsetDevices();

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
        connect(m_device, &HeadsetDevice::teamsButton, this, [this]() {
            if (isEnabled()) {
                emit teamsButton();
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
            m_device->syncDateAndTime();

            auto &ui = UserInfo::instance();
            QString displayName = ui.getDisplayName();

            if (displayName.isEmpty()) {
                connect(
                        &ui, &UserInfo::displayNameChanged, this,
                        [this]() {
                            m_device->setLocalUserName(UserInfo::instance().getDisplayName());
                        },
                        Qt::SingleShotConnection);
            } else {
                m_device->setLocalUserName(displayName);
            }

            // Test --->
            // m_device->selectScreen(ReportDescriptorEnums::TeamsScreenSelect::HomeScreen);
            // m_device->setOtherUserName("Max Mustermann");
            // m_device->setOtherUserNumber("+4912345678");
            // m_device->setSubject("Meeting XYZ");
            // Additional icons -> FWD, Mute, Speaker, Missed Call, Voice Mail
            // <---
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

void HeadsetDeviceProxy::syncDateAndTime()
{
    if (m_device) {
        m_device->syncDateAndTime();
    }
}

void HeadsetDeviceProxy::setLocalUserName(const QString &name)
{
    if (m_device) {
        m_device->setLocalUserName(name);
    }
}

void HeadsetDeviceProxy::setLocalUserNumber(const QString &number)
{
    if (m_device) {
        m_device->setLocalUserNumber(number);
    }
}

void HeadsetDeviceProxy::setLocalUserStatus(const QString &status)
{
    if (m_device) {
        m_device->setLocalUserStatus(status);
    }
}

void HeadsetDeviceProxy::setOtherUserName(const QString &name)
{
    if (m_device) {
        m_device->setOtherUserName(name);
    }
}

void HeadsetDeviceProxy::setOtherUserNumber(const QString &number)
{
    if (m_device) {
        m_device->setOtherUserNumber(number);
    }
}

void HeadsetDeviceProxy::setSubject(const QString &subject)
{
    if (m_device) {
        m_device->setSubject(subject);
    }
}

void HeadsetDeviceProxy::selectScreen(ReportDescriptorEnums::TeamsScreenSelect screen, bool clear,
                                      bool backlight)
{
    if (m_device) {
        m_device->selectScreen(screen, clear, backlight);
    }
}

void HeadsetDeviceProxy::setPresenceIcon(ReportDescriptorEnums::TeamsPresenceIcon icon)
{
    if (m_device) {
        m_device->setPresenceIcon(icon);
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
