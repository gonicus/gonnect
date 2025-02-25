#include "BusylightDeviceManager.h"

#include "LuxaforFlag.h"
#include "KuandoOmega.h"

BusylightDeviceManager::BusylightDeviceManager(QObject *parent) : QObject{ parent } { }

bool BusylightDeviceManager::createBusylightDevice(const hid_device_info &deviceInfo)
{
    IBusylightDevice *device = nullptr;
    const quint16 vendor = deviceInfo.vendor_id;
    const quint16 product = deviceInfo.product_id;

    if (vendor == 0x04D8 && product == 0xF372) {
        device = new LuxaforFlag(deviceInfo, this);

    } else if (vendor == 0x27BB && (product == 0x3BCD || product == 0x3BCF)) {
        device = new KuandoOmega(deviceInfo, this);
    }

    if (device) {
        device->open();
        m_devices.append(device);
    }
    return device;
}

void BusylightDeviceManager::clearDevices()
{
    qDeleteAll(m_devices);
    m_devices.clear();
}

void BusylightDeviceManager::switchOn(QColor color) const
{
    for (auto device : std::as_const(m_devices)) {
        device->switchOn(color);
    }
}

void BusylightDeviceManager::switchOff() const
{
    for (auto device : std::as_const(m_devices)) {
        device->switchOff();
    }
}

void BusylightDeviceManager::startBlinking(QColor color) const
{
    for (auto device : std::as_const(m_devices)) {
        device->startBlinking(color);
    }
}

void BusylightDeviceManager::stopBlinking() const
{
    for (auto device : std::as_const(m_devices)) {
        device->stopBlinking();
    }
}
