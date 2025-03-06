#include <QDebug>
#include <QMutexLocker>
#include <QLoggingCategory>
#include <libusb.h>
#include "HeadsetDevice.h"

Q_LOGGING_CATEGORY(lcHeadset, "gonnect.usb.headset")

using namespace std::chrono_literals;

HeadsetDevice::HeadsetDevice(const hid_device_info *deviceInfo, QObject *parent)
    : IHeadsetDevice(parent)
{
    Q_CHECK_PTR(deviceInfo);

    m_productName = QString::fromWCharArray(deviceInfo->product_string);
    m_path = deviceInfo->path;

    m_eventHandler.setSingleShot(false);
    m_eventHandler.setInterval(250ms);
    connect(&m_eventHandler, &QTimer::timeout, this, &HeadsetDevice::processEvents);
}

HeadsetDevice::~HeadsetDevice()
{
    close();
}

bool HeadsetDevice::open()
{
    if (m_device) {
        return true;
    }

    hid_device *device = hid_open_path(m_path.toStdString().c_str());
    if (device) {
        m_device = device;
        m_eventHandler.start();
        m_isOpen = true;
    } else {
        qCCritical(lcHeadset) << "failed to open headset device";
    }

    return device;
}

void HeadsetDevice::close()
{
    m_eventHandler.stop();

    if (m_device) {
        hid_close(m_device);
        m_device = nullptr;
    }

    m_isOpen = false;
}

void HeadsetDevice::send(quint8 reportId, unsigned data)
{
    if (!m_device) {
        qCCritical(lcHeadset) << "device not open during attempt to send data";
        return;
    }

    unsigned char buf[2];
    buf[0] = reportId;
    buf[1] = data;

    qCDebug(lcHeadset) << "Sending report" << QString::asprintf("0x%02X", reportId) << "with data" << QString::asprintf("0x%08X", data);

    hid_write(m_device, buf, sizeof(buf));
}

void HeadsetDevice::setIdle()
{
    static const QList<UsageId> usageIds = { UsageId::LED_OffHook, UsageId::LED_Mute,
                                             UsageId::LED_Ring, UsageId::LED_Hold,
                                             UsageId::Telephony_Ringer };

    QHash<quint8, unsigned> reportVals;

    for (const auto usageId : usageIds) {
        const auto &usage = m_hidUsages.value(usageId);
        reportVals.insert(usage.reportId,
                          reportVals.value(usage.reportId, 0) | (0 << usage.bitPosition));
    }

    QHashIterator it(reportVals);
    while (it.hasNext()) {
        it.next();
        send(it.key(), it.value());
    }
}

void HeadsetDevice::setUsageInfos(const QHash<UsageId, UsageInfo> &infos)
{
    for (const auto &usageInfo : infos) {
        m_inputReportIds.insert(usageInfo.reportId);
    }

    if (m_inputReportIds.empty()) {
        qCCritical(lcHeadset) << "No report id found in HID descriptor.";
        return;
    }

    m_hidUsages = infos;
}

void HeadsetDevice::setBusyLine(bool flag)
{
    if (!m_hidUsages.contains(UsageId::LED_OffHook)) {
        qCInfo(lcHeadset) << "Busy line is not supported by this device";
        return;
    }

    const auto &usage = m_hidUsages.value(UsageId::LED_OffHook);
    const unsigned bitValue = 1 << usage.bitPosition;
    unsigned value = currentFlags(usage.reportId);

    if (flag) {
        value |= bitValue;
    } else {
        value &= ~bitValue;
    }

    send(usage.reportId, value);
}

void HeadsetDevice::setMute(bool flag)
{
    if (!m_hidUsages.contains(UsageId::LED_Mute)) {
        qCInfo(lcHeadset) << "Muting is not supported by this device";
        return;
    }

    const auto &usage = m_hidUsages.value(UsageId::LED_Mute);
    const unsigned bitValue = 1 << usage.bitPosition;
    unsigned value = currentFlags(usage.reportId);

    if (flag) {
        value |= bitValue;
    } else {
        value &= ~bitValue;
    }

    send(usage.reportId, value);
}

void HeadsetDevice::setRing(bool flag)
{
    if (!m_hidUsages.contains(UsageId::LED_Ring)
        && !m_hidUsages.contains(UsageId::Telephony_Ringer)) {
        qCInfo(lcHeadset) << "Ringing is not supported by this device";
        return;
    }

    m_ringing = flag;

    if (m_hidUsages.contains(UsageId::LED_Ring)) {
        const auto &usage = m_hidUsages.value(UsageId::LED_Ring);
        unsigned value = currentFlags(usage.reportId);
        const unsigned v = 1 << usage.bitPosition;

        if (flag) {
            value |= v;
        } else {
            value &= ~v;
        }

        send(usage.reportId, value);
    }

    if (m_hidUsages.contains(UsageId::Telephony_Ringer)) {
        const auto &usage = m_hidUsages.value(UsageId::Telephony_Ringer);
        unsigned value = currentFlags(usage.reportId);
        const unsigned v = 1 << usage.bitPosition;

        if (flag) {
            value |= v;
        } else {
            value &= ~v;
        }

        send(usage.reportId, value);
    }
}

void HeadsetDevice::setHold(bool flag)
{
    if (!m_hidUsages.contains(UsageId::LED_Hold)) {
        qCInfo(lcHeadset) << "Hold is not supported by this device";
        return;
    }

    const auto &usageHold = m_hidUsages.value(UsageId::LED_Hold);
    const unsigned holdValue = 1 << usageHold.bitPosition;
    unsigned value = currentFlags(usageHold.reportId);

    if (flag) {
        value |= holdValue;
    } else {
        value &= ~holdValue;
    }

    m_hold = flag;
    send(usageHold.reportId, value);
}

unsigned HeadsetDevice::currentFlags(const quint32 reportId) const
{
    unsigned value = 0;

    if (m_hidUsages.contains(UsageId::LED_OffHook) && (m_line || m_hookSwitch)) {
        const auto &u = m_hidUsages.value(UsageId::LED_OffHook);
        if (u.reportId == reportId) {
            value |= 1 << u.bitPosition;
        }
    }
    if (m_hidUsages.contains(UsageId::LED_Mute) && m_muted) {
        const auto &u = m_hidUsages.value(UsageId::LED_Mute);
        if (u.reportId == reportId) {
            value |= 1 << u.bitPosition;
        }
    }
    if (m_hidUsages.contains(UsageId::LED_Ring) && m_ringing) {
        const auto &u = m_hidUsages.value(UsageId::LED_Ring);
        if (u.reportId == reportId) {
            value |= 1 << u.bitPosition;
        }
    }
    if (m_hidUsages.contains(UsageId::LED_Hold) && m_hold) {
        const auto &u = m_hidUsages.value(UsageId::LED_Hold);
        if (u.reportId == reportId) {
            value |= 1 << u.bitPosition;
        }
    }

    qCDebug(lcHeadset) << "Current flags" << QString::asprintf("0x%08X", value);

    return value;
}

void HeadsetDevice::processEvents()
{
    unsigned char data[64];
    memset(data, 0, sizeof(data));

    auto len = hid_read_timeout(m_device, data, sizeof(data), 10);

    if (len > 2 && m_inputReportIds.contains(data[0])) {
        quint8 reportId = data[0];
        unsigned value = data[1];
        qCInfo(lcHeadset).noquote().nospace()
                << "Found releveant report with report id 0x" << QString::number(reportId, 16);

        // Hook switch
        if (m_hidUsages.contains(UsageId::Telephony_HookSwitch)) {
            const auto &usage = m_hidUsages.value(UsageId::Telephony_HookSwitch);
            if (usage.reportId == reportId) {
                bool _hookSwitch = value & (1 << usage.bitPosition);
                if (m_hookSwitch != _hookSwitch) {
                    m_hookSwitch = _hookSwitch;
                    emit hookSwitch();
                }
            }
        }

        // Muted
        if (m_hidUsages.contains(UsageId::Telephony_PhoneMute)) {
            const auto &usage = m_hidUsages.value(UsageId::Telephony_PhoneMute);
            if (usage.reportId == reportId && (value & (1 << usage.bitPosition))) {
                m_muted = !m_muted;
                emit mute();
            }
        }

        // Line
        if (m_hidUsages.contains(UsageId::Telephony_LineBusyTone)) {
            const auto &usage = m_hidUsages.value(UsageId::Telephony_LineBusyTone);
            if (usage.reportId == reportId) {
                bool _line = value & (1 << usage.bitPosition);
                if (m_line != _line) {
                    m_line = _line;
                    emit busyLine();
                }
            }
        }

        // Flashing
        if (m_hidUsages.contains(UsageId::Telephony_Flash)) {
            const auto &usage = m_hidUsages.value(UsageId::Telephony_Flash);
            if (usage.reportId == reportId) {
                bool _flash = value & (1 << usage.bitPosition);
                if (m_flash != _flash) {
                    m_flash = _flash;
                    emit flash();
                }
            }
        }
    }
}
