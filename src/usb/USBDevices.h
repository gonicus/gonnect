#pragma once
#ifdef WIN32
#  include <winsock2.h>
#endif

#include <hidapi.h>
#include <libusb.h>
#include <QObject>
#include <QTimer>

class HeadsetDevice;
class HeadsetDeviceProxy;

class USBDevices : public QObject
{
    Q_OBJECT

public:
    static USBDevices &instance()
    {
        static USBDevices *_instance = nullptr;

        if (_instance == nullptr) {
            _instance = new USBDevices();
            _instance->initialize();
        }

        return *_instance;
    }

    void initialize();
    void shutdown();

    int hotplugHandler(libusb_context *ctx, libusb_device *device, libusb_hotplug_event event,
                       void *user_data);

    QList<HeadsetDevice *> headsetDevices() const { return m_headsetDevices; }

    HeadsetDeviceProxy *getHeadsetDeviceProxy();

    ~USBDevices() { shutdown(); }

Q_SIGNALS:
    void devicesChanged();

private Q_SLOTS:
    void processUsbEvents();

private:
    explicit USBDevices(QObject *parent = nullptr);

    void refresh();
    void clearDevices();

    HeadsetDevice *parseReportDescriptor(const hid_device_info *deviceInfo);
    HeadsetDevice *parseReportDescriptor(const hid_device_info *deviceInfo,
                                         unsigned char *descriptor, int len);

    QTimer m_refreshTicker;
    QTimer m_refreshDebouncer;

    QList<HeadsetDevice *> m_headsetDevices;
    HeadsetDeviceProxy *m_proxy = nullptr;
    libusb_context *m_ctx = nullptr;

    bool m_hotplugSupported = false;
    bool m_initialized = false;
};
