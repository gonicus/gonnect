#pragma once

#include <QObject>
#include <QColor>
#include <QTimer>
#include <QtQml/qqml.h>
#include <QtQml/qqmlregistration.h>

#include "hidapi.h"

class IBusylightDevice;

class BusylightDeviceManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasStreamingLight READ hasStreamingLight NOTIFY streamingLightsUpdated FINAL)
    Q_PROPERTY(bool streamingLightActive READ streamingLightActive NOTIFY streamingLightActiveChanged FINAL)
    Q_PROPERTY(unsigned streamingLightBrightness READ streamingLightBrightness NOTIFY streamingLightBrightnessChanged FINAL)

public:
    static BusylightDeviceManager &instance()
    {
        static BusylightDeviceManager *_instance = nullptr;
        if (!_instance) {
            _instance = new BusylightDeviceManager;
        }
        return *_instance;
    }

    bool createBusylightDevice(const struct hid_device_info &deviceInfo);
    void removeDevice(IBusylightDevice *dev);

    void clearDevices();

    void switchOn(QColor color) const;
    void switchOff() const;

    void startBlinking(QColor color) const;
    void stopBlinking() const;

    void switchStreamlightOn();
    void switchStreamlightOff();

    Q_INVOKABLE void toggleStreamingLight();

    Q_INVOKABLE void setStreamingLightBrightness(unsigned value);
    unsigned streamingLightBrightness() const { return m_brightness; }

    bool hasStreamingLight() const { return m_hasStreamingLight; }
    bool streamingLightActive() const { return m_streamingLightActive; }

    QList<IBusylightDevice *> devices() const { return m_devices; }

private Q_SLOTS:
    void updateBusylightState();

Q_SIGNALS:
    void streamingLightsUpdated();
    void streamingLightActiveChanged();
    void streamingLightBrightnessChanged(unsigned value);

private:
    explicit BusylightDeviceManager(QObject *parent = nullptr);
    void refreshOwnCapabilities();
    void applyStreamingLightBrightness();

    QTimer m_brightnessUpdateDebouncer;

    QList<IBusylightDevice *> m_devices;

    unsigned m_brightness = 0;

    bool m_hasStreamingLight = false;
    bool m_streamingLightActive = false;
};


class BusylightDeviceManagerWrapper
{
    Q_GADGET
    QML_FOREIGN(BusylightDeviceManager)
    QML_NAMED_ELEMENT(BusylightDeviceManager)
    QML_SINGLETON

public:
    static BusylightDeviceManager *create(QQmlEngine *, QJSEngine *) { return &BusylightDeviceManager::instance(); }

private:
    BusylightDeviceManagerWrapper() = default;
};
