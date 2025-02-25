#include <QDebug>
#include <QThread>
#include <QMutexLocker>
#include <QLoggingCategory>
#include <cmath>
#include "ReportDescriptorParser.h"
#include "ReportDescriptorEnums.h"
#include "ReportDescriptorStructs.h"
#include "USBDevices.h"
#include "HeadsetDevice.h"
#include "HeadsetDeviceProxy.h"
#include "BusylightDeviceManager.h"
#include "IBusylightDevice.h"

Q_LOGGING_CATEGORY(lcHeadsets, "gonnect.usb.headsets")

using namespace std::chrono_literals;

static QMutex s_enumerateMutex; // clazy:exclude=non-pod-global-static
static libusb_hotplug_callback_handle s_hotplugHandle;

static int LIBUSB_CALL hotplugCallback(libusb_context *ctx, libusb_device *device,
                                       libusb_hotplug_event event, void *user_data)
{
    return USBDevices::instance().hotplugHandler(ctx, device, event, user_data);
}

USBDevices::USBDevices(QObject *parent) : QObject(parent)
{
    QThread *t = new QThread(this);

    m_refreshDebouncer.setSingleShot(true);
    m_refreshDebouncer.setInterval(2s);
    connect(&m_refreshDebouncer, &QTimer::timeout, this, &USBDevices::refresh);

    m_refreshTicker.setInterval(250ms);
    m_refreshTicker.moveToThread(t);
    m_refreshTicker.connect(t, SIGNAL(started()), SLOT(start()));
    m_refreshTicker.connect(t, SIGNAL(finished()), SLOT(stop()));
    connect(&m_refreshTicker, &QTimer::timeout, this, &USBDevices::processUsbEvents);

    int res = libusb_init(&m_ctx);
    if (res < 0) {
        libusb_exit(m_ctx);
        qCWarning(lcHeadsets) << "failed to initialize libusb:" << res;
        return;
    }

    m_hotplugSupported = libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG) != 0;

    if (m_hotplugSupported) {
        res = libusb_hotplug_register_callback(
                m_ctx,
                static_cast<libusb_hotplug_event>(
                        (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT)),
                LIBUSB_HOTPLUG_NO_FLAGS, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
                LIBUSB_HOTPLUG_MATCH_ANY,
                reinterpret_cast<libusb_hotplug_callback_fn>(hotplugCallback), nullptr,
                &s_hotplugHandle);
        if (LIBUSB_SUCCESS != res) {
            libusb_exit(m_ctx);
            qCWarning(lcHeadsets) << "failed to register hotplug callback:" << res;
            return;
        }

        t->start();
    } else {
        qCWarning(lcHeadsets) << "USB hotplug not supported - refresh not available";
    }
}

void USBDevices::processUsbEvents()
{
    if (m_hotplugSupported) {
        timeval t = { 0, 0 };
        libusb_handle_events_timeout_completed(m_ctx, &t, Q_NULLPTR);
    }
}

void USBDevices::initialize()
{
    if (!m_initialized) {
        refresh();
        m_initialized = true;
    }
}

void USBDevices::shutdown()
{
    if (!m_ctx) {
        return;
    }

    m_refreshTicker.thread()->exit();
    m_refreshTicker.thread()->wait();
    m_refreshTicker.thread()->deleteLater();

    if (m_hotplugSupported) {
        timeval t = { 0, 0 };
        libusb_handle_events_timeout_completed(m_ctx, &t, nullptr);
        libusb_hotplug_deregister_callback(m_ctx, s_hotplugHandle);
    }

    libusb_exit(m_ctx);
    m_ctx = nullptr;

    if (m_proxy) {
        m_proxy->close();
        m_proxy = nullptr;
    }

    clearDevices();
}

int USBDevices::hotplugHandler(libusb_context *, libusb_device *device, libusb_hotplug_event event,
                               void *)
{
    quint8 bus = libusb_get_bus_number(device);

    QStringList portStrings;
    quint8 ports[7];
    quint8 num = libusb_get_port_numbers(device, ports, sizeof(ports));
    for (auto i = 0; i < num; i++) {
        portStrings.append(QString::number(ports[i]));
    }

    QString path = QString::number(bus) + "-" + portStrings.join('.') + ":";

    if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
        qCDebug(lcHeadsets) << "device" << path << "added";
        m_refreshDebouncer.start();

    } else if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT) {
        qCDebug(lcHeadsets) << "device" << path << "removed";
        m_refreshDebouncer.start();

    } else {
        return -1;
    }

    return 0;
};

void USBDevices::refresh()
{
    QMutexLocker lock(&s_enumerateMutex);
    QString lastPath;
    auto &busylightDeviceManager = BusylightDeviceManager::instance();

    clearDevices();

    struct hid_device_info *devs, *deviceInfo;

    deviceInfo = devs = hid_enumerate(0, 0);

    for (; deviceInfo; deviceInfo = deviceInfo->next) {

        QString path = deviceInfo->path;
        if (path == lastPath) {
            continue;
        }

        lastPath = path;

        if (!busylightDeviceManager.createBusylightDevice(*deviceInfo)) {
            HeadsetDevice *hd = parseReportDescriptor(deviceInfo);
            if (hd) {
                m_headsetDevices.push_back(hd);
            }
        }
    }

    hid_free_enumeration(devs);

    emit devicesChanged();
}

void USBDevices::clearDevices()
{
    qDeleteAll(m_headsetDevices);
    m_headsetDevices.clear();

    BusylightDeviceManager::instance().clearDevices();
}

HeadsetDevice *USBDevices::parseReportDescriptor(const hid_device_info *deviceInfo)
{
    unsigned char descriptor[HID_API_MAX_REPORT_DESCRIPTOR_SIZE];
    hid_device *device = hid_open_path(deviceInfo->path);

    if (!device) {
        return nullptr;
    }

    int len = hid_get_report_descriptor(device, descriptor, sizeof(descriptor));
    HeadsetDevice *hd = parseReportDescriptor(deviceInfo, descriptor, len);

    hid_close(device);
    return hd;
}

HeadsetDevice *USBDevices::parseReportDescriptor(const hid_device_info *deviceInfo,
                                                 unsigned char *descriptor, int len)
{
    const auto byteArr = QByteArray::fromRawData(reinterpret_cast<const char *>(descriptor), len);

    ReportDescriptorParser parser;

    std::shared_ptr<ApplicationCollection> appCollection;

    try {
        appCollection = parser.parse(byteArr);
    } catch (...) {
        QString ps = QString::fromWCharArray(deviceInfo->product_string);
        qCWarning(lcHeadsets, "failed to parse report descriptor for %s [%s]", deviceInfo->path,
                  ps.toStdString().c_str());
        return nullptr;
    }

    qCritical() << "====>" << appCollection.get();

    if (!appCollection) {
        return nullptr;
    }

    static const QList<UsageId> usageIdsOfInterest = {
        UsageId::Telephony_HookSwitch,
        UsageId::Telephony_PhoneMute,
        UsageId::Telephony_LineBusyTone,
        UsageId::Telephony_Flash,
        UsageId::Telephony_Ringer,
        UsageId::LED_OffHook,
        UsageId::LED_Mute,
        UsageId::LED_Ring,
        UsageId::LED_Hold,
        UsageId::Vendor_LEDCommand,
    };

    QHash<UsageId, UsageInfo> usageInfos;

    QString ps = QString::fromWCharArray(deviceInfo->product_string);
    qCDebug(lcHeadsets, "HID headset %s [%s] found:", deviceInfo->path, ps.toStdString().c_str());

    for (const auto usageId : usageIdsOfInterest) {
        const auto res = appCollection->findUsage(usageId);
        if (res.isValid()) {
            qCDebug(lcHeadsets).noquote()
                    << QString("  %1 (bit position %2, report id 0x%3)")
                               .arg(ReportDescriptorEnums::toString(res.usage->id))
                               .arg(res.bitPositionInReport)
                               .arg(res.report->id, 2, 16, QChar('0'));
            usageInfos.insert(usageId,
                              { res.bitPositionInReport, static_cast<quint8>(res.report->id) });
        }
    }

    HeadsetDevice *hd = new HeadsetDevice(deviceInfo, this);
    hd->setUsageInfos(usageInfos);
    return hd;
}

HeadsetDeviceProxy *USBDevices::getHeadsetDeviceProxy()
{
    if (!m_proxy) {
        m_proxy = new HeadsetDeviceProxy(this);
    }

    return m_proxy;
}
