#include "BusylightDeviceManager.h"

#include "LitraBeamLX.h"
#include "LitraGlow.h"
#include "LuxaforFlag.h"
#include "KuandoOmega.h"
#include "GlobalCallState.h"
#include "GlobalMuteState.h"

#include <QSet>

using namespace std::chrono_literals;

BusylightDeviceManager::BusylightDeviceManager(QObject *parent) : QObject{ parent }
{
    m_brightnessUpdateDebouncer.setSingleShot(true);
    m_brightnessUpdateDebouncer.setInterval(800ms);
    connect(&m_brightnessUpdateDebouncer, &QTimer::timeout, this,
            &BusylightDeviceManager::applyStreamingLightBrightness);

    connect(&GlobalCallState::instance(), &GlobalCallState::globalCallStateChanged, this,
            &BusylightDeviceManager::updateBusylightState);
    connect(&GlobalMuteState::instance(), &GlobalMuteState::isMutedChangedWithTag, this,
            &BusylightDeviceManager::updateBusylightState);
    updateBusylightState();
}

bool BusylightDeviceManager::createBusylightDevice(const hid_device_info &deviceInfo)
{
    IBusylightDevice *device = nullptr;
    const quint16 vendor = deviceInfo.vendor_id;
    const quint16 product = deviceInfo.product_id;

    if (vendor == 0x04D8 && product == 0xF372) {
        device = new LuxaforFlag(deviceInfo, this);

    } else if (vendor == 0x27BB && (product == 0x3BCD || product == 0x3BCF)) {
        device = new KuandoOmega(deviceInfo, this);

    } else if (vendor == 0x046D && product == 0xC900) {
        device = new LitraGlow(deviceInfo, this);

    } else if (vendor == 0x046D && product == 0xC903) {
        device = new LitraBeamLX(deviceInfo, this);
    }

    if (device) {
        device->open();
        m_devices.append(device);
        refreshOwnCapabilities();
    }

    return device;
}

void BusylightDeviceManager::removeDevice(IBusylightDevice *dev)
{
    m_devices.removeAll(dev);
    delete dev;
    refreshOwnCapabilities();
}

void BusylightDeviceManager::clearDevices()
{
    qDeleteAll(m_devices);
    m_devices.clear();
    refreshOwnCapabilities();
}

void BusylightDeviceManager::switchOn(QColor color) const
{
    for (auto device : std::as_const(m_devices)) {
        if (device->supportedCommands().contains(
                    IBusylightDevice::SupportedCommands::BusylightOnOff)) {
            device->switchOn(color);
        }
    }
}

void BusylightDeviceManager::switchOff() const
{
    for (auto device : std::as_const(m_devices)) {
        if (device->supportedCommands().contains(
                    IBusylightDevice::SupportedCommands::BusylightOnOff)) {
            device->switchOff();
        }
    }
}

void BusylightDeviceManager::applyStreamingLightBrightness()
{
    for (auto device : std::as_const(m_devices)) {
        if (device->supportedCommands().contains(
                    IBusylightDevice::SupportedCommands::StreamlightBrightness)) {
            device->setStreaminglightBrightness(m_brightness);
        }
    }

    Q_EMIT streamingLightBrightnessChanged(m_brightness);
}

void BusylightDeviceManager::startBlinking(QColor color) const
{
    for (auto device : std::as_const(m_devices)) {
        if (device->supportedCommands().contains(
                    IBusylightDevice::SupportedCommands::BusylightOnOff)) {
            device->startBlinking(color);
        }
    }
}

void BusylightDeviceManager::stopBlinking() const
{
    for (auto device : std::as_const(m_devices)) {
        if (device->supportedCommands().contains(
                    IBusylightDevice::SupportedCommands::BusylightOnOff)) {
            device->stopBlinking();
        }
    }
}

void BusylightDeviceManager::switchStreamlightOn()
{
    if (!m_streamingLightActive) {
        m_streamingLightActive = true;

        for (auto device : std::as_const(m_devices)) {
            if (device->supportedCommands().contains(
                        IBusylightDevice::SupportedCommands::StreamlightOnOff)) {
                device->switchStreamlight(true);
            }
        }

        Q_EMIT streamingLightActiveChanged();
    }
}

void BusylightDeviceManager::switchStreamlightOff()
{
    if (m_streamingLightActive) {
        m_streamingLightActive = false;

        for (auto device : std::as_const(m_devices)) {
            if (device->supportedCommands().contains(
                        IBusylightDevice::SupportedCommands::StreamlightOnOff)) {
                device->switchStreamlight(false);
            }
        }

        Q_EMIT streamingLightActiveChanged();
    }
}

void BusylightDeviceManager::toggleStreamingLight()
{
    if (m_streamingLightActive) {
        switchStreamlightOff();
    } else {
        switchStreamlightOn();
    }
}

void BusylightDeviceManager::refreshOwnCapabilities()
{
    bool hasStreamingLight = false;

    for (auto device : std::as_const(m_devices)) {
        if (device->supportedCommands().contains(
                    IBusylightDevice::SupportedCommands::StreamlightOnOff)) {
            hasStreamingLight = true;
            break;
        }
    }

    if (hasStreamingLight != m_hasStreamingLight) {
        m_hasStreamingLight = hasStreamingLight;
        Q_EMIT streamingLightsUpdated();
    }
}

void BusylightDeviceManager::setStreamingLightBrightness(unsigned value) {
    if (value <= 100 && m_brightness != value) {
        m_brightness = value;
        m_brightnessUpdateDebouncer.start();
    }
}


void BusylightDeviceManager::updateBusylightState()
{
    const auto state = GlobalCallState::instance().globalCallState();
    typedef ICallState::State State;

    const bool isRinging = state & State::RingingIncoming;
    const bool isCallActive = state & State::CallActive;
    const bool isOnHold = state & State::OnHold;
    const bool isMuted = GlobalMuteState::instance().isMuted();
    const bool isVideoActive = state & State::VideoActive;

    if (isRinging) {
        startBlinking(Qt::GlobalColor::green);
        return;
    }

    if (!isCallActive) {
        stopBlinking();
        switchOff();
        switchStreamlightOff();
        return;
    }

    QColor color(Qt::GlobalColor::red);
    if (isMuted) {
        color.setRgb(255, 165, 0);
    }

    if (isOnHold) {
        startBlinking(color);
    } else {
        switchOn(color);
    }

    if (isVideoActive) {
        switchStreamlightOn();
    } else {
        switchStreamlightOff();
    }
}
