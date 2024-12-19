#pragma once
#include <hidapi.h>
#include <libusb.h>
#include <QObject>
#include <QTimer>

class HeadsetDevice;
class HeadsetDeviceProxy;

class HeadsetDevices : public QObject
{
    Q_OBJECT

public:
    static HeadsetDevices &instance()
    {
        static HeadsetDevices *_instance = nullptr;

        if (_instance == nullptr) {
            _instance = new HeadsetDevices();
            _instance->initialize();
        }

        return *_instance;
    }

    void initialize();
    void shutdown();

    int hotplugHandler(libusb_context *ctx, libusb_device *device, libusb_hotplug_event event,
                       void *user_data);

    QList<HeadsetDevice *> devices() const { return m_devices; }

    HeadsetDeviceProxy *getProxy();

    ~HeadsetDevices() { shutdown(); }

signals:
    void devicesChanged();

private slots:
    void processUsbEvents();

private:
    explicit HeadsetDevices(QObject *parent = nullptr);

    void refresh();

    HeadsetDevice *parseReportDescriptor(const hid_device_info *deviceInfo);
    HeadsetDevice *parseReportDescriptor(const hid_device_info *deviceInfo,
                                         unsigned char *descriptor, int len);

    QTimer m_refreshTicker;
    QTimer m_refreshDebouncer;

    QList<HeadsetDevice *> m_devices;
    HeadsetDeviceProxy *m_proxy = nullptr;
    libusb_context *m_ctx = nullptr;

    bool m_hotplugSupported = false;
    bool m_initialized = false;
};
