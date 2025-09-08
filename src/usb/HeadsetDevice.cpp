#include <QDebug>
#include <QMutexLocker>
#include <QLoggingCategory>
#include <QDateTime>
#include <libusb.h>
#include "HeadsetDevice.h"
#include "GlobalInfo.h"

Q_LOGGING_CATEGORY(lcHeadset, "gonnect.usb.headset")

using namespace std::chrono_literals;

QDebug operator<<(QDebug debug, const UsageInfo &usageInfo)
{
    QDebugStateSaver saver(debug);

    const auto reportHex = QString("0x%1").arg(usageInfo.reportId, 2, 16, QChar('0'));
    debug.noquote().nospace() << "UsageInfo(report: " << reportHex
                              << ", bit position: " << usageInfo.bitPosition
                              << ", size: " << usageInfo.size
                              << ", value: " << (1 << usageInfo.bitPosition) << ")";
    return debug;
}

HeadsetDevice::HeadsetDevice(const hid_device_info *deviceInfo, QObject *parent)
    : IHeadsetDevice(parent)
{
    Q_CHECK_PTR(deviceInfo);

    m_productName = QString::fromWCharArray(deviceInfo->product_string);
    m_path = deviceInfo->path;

    m_ignoreHookTimer.setSingleShot(true);
    m_ignoreHookTimer.setInterval(500ms);

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
    if (!useHeadset()) {
        return false;
    }

    if (m_device) {
        return true;
    }

    hid_device *device = hid_open_path(m_path.toStdString().c_str());
    if (device) {
        m_device = device;
        m_eventHandler.start();
        m_isOpen = true;

        // Read Teams info if available
        if (m_teamsUsageMapping.contains(UsageId::Teams_VendorExtension)) {
            unsigned char buf[256];
            memset(buf, 0, sizeof(buf));

            // Read vendor extension from device
            buf[0] = m_teamsUsageMapping[UsageId::Teams_VendorExtension];
            auto res = hid_get_feature_report(device, buf, sizeof(buf));
            if (res >= 5) {
                quint32 vendorId = buf[1] + (buf[2] << 8);
                quint32 version = buf[3] + (buf[4] << 8);
                m_displaySupported = vendorId == 0x045e && version == 0x0200;
            }

            // If we've display support, read the display configuration from device
            if (m_displaySupported
                && m_teamsUsageMapping.contains(UsageId::Teams_DisplayAttributes)) {
                memset(buf, 0, sizeof(buf));
                buf[0] = m_teamsUsageMapping[UsageId::Teams_DisplayAttributes];
                auto res = hid_get_feature_report(device, buf, sizeof(buf));
                if (res >= 5) {
                    m_displayRows = buf[1];
                    m_displayCols = buf[2];
                    m_displayFieldSupportIndex = buf[3] + (buf[4] << 8);
                }
            }

            // No idea what that means, but it enables the Teams button on the device.
            // Sadly the ASP documentation is only available for Microsoft partners.
            sendASP(0);
            sendASP(0x10);
        }
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
    if (!useHeadset()) {
        return;
    }
    if (!m_device) {
        qCCritical(lcHeadset) << "device not open during attempt to send data";
        return;
    }

    unsigned char buf[2];
    buf[0] = reportId;
    buf[1] = data;

    qCDebug(lcHeadset) << "Sending report" << QString::asprintf("0x%02X", reportId) << "with data"
                       << QString::asprintf("0x%08X", data);

    hid_write(m_device, buf, sizeof(buf));
}

void HeadsetDevice::setIdle()
{
    static const QList<UsageId> usageIds = { UsageId::LED_OffHook, UsageId::LED_Mute,
                                             UsageId::LED_Ring, UsageId::LED_Hold,
                                             UsageId::Telephony_Ringer };

    QHash<quint8, unsigned> reportVals;
    QList<UsageInfo> usageInfos;

    for (const auto usageId : usageIds) {
        if (m_hidUsages.contains(usageId)) {
            const auto &usage = m_hidUsages.value(usageId);
            usageInfos.append(usage);
            reportVals.insert(usage.reportId,
                              reportVals.value(usage.reportId, 0) | (0 << usage.bitPosition));
        }
    }

    qCInfo(lcHeadset) << "Sending 'idle' state with these usages:";
    for (const auto &usageInfo : std::as_const(usageInfos)) {
        qCInfo(lcHeadset) << " " << usageInfo;
    }

    qCInfo(lcHeadset) << "Sending...";
    QHashIterator it(reportVals);
    while (it.hasNext()) {
        it.next();
        qCInfo(lcHeadset).noquote() << QString("  reportId 0x%1, value 0x%2")
                                               .arg(it.key(), 2, 16, QChar('0'))
                                               .arg(it.value(), 8, 16, QChar('0'));
        send(it.key(), it.value());
    }
}

void HeadsetDevice::setUsageInfos(const QHash<UsageId, UsageInfo> &infos)
{
    m_inputReportIds.clear();

    qCDebug(lcHeadset) << "Setting usage infos for headset device" << m_productName;

    for (const auto &usageInfo : infos) {
        qCDebug(lcHeadset) << "  " << usageInfo;
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
    if (GlobalInfo::instance().isWorkaroundActive(GlobalInfo::WorkaroundId::GOW_001)) {
        m_ignoreHookTimer.start();
    }

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

    qCInfo(lcHeadset) << "Sending 'busy line' state with value" << flag << "to headset with usage"
                      << usage;
    send(usage.reportId, value);
}

void HeadsetDevice::setMute(bool flag)
{
    if (!m_hidUsages.contains(UsageId::LED_Mute)) {
        qCInfo(lcHeadset) << "Muting is not supported by this device";
        return;
    }

    if (m_appSettings.value("generic/disableMutePropagation", false).toBool()) {
        qCDebug(lcHeadset) << "Mute propagation is disabled - skipping";
        return;
    }

    if (m_ignoreNextMuteUpdate) {
        m_ignoreNextMuteUpdate = false;
        qCDebug(lcHeadset).noquote().nospace() << "Ignoring setMute(" << flag << ") call";
        return;
    }

    m_muted = flag;

    if (m_displaySupported && m_teamsUsageMapping.contains(UsageId::Teams_IconsControl)) {
        unsigned char buf[3];
        buf[0] = m_teamsUsageMapping[UsageId::Teams_IconsControl];
        buf[1] = (quint8)m_presenceIcon & 0b1111;
        buf[2] = flag ? 0b00010000 : 0b00110000;

        qCInfo(lcHeadset) << "Sending 'mute' state with value" << flag
                          << "to headset with display specification usage"
                          << UsageId::Teams_IconsControl;
        hid_write(m_device, buf, sizeof(buf));
    } else {
        const auto &usage = m_hidUsages.value(UsageId::LED_Mute);
        const unsigned bitValue = 1 << usage.bitPosition;
        unsigned value = currentFlags(usage.reportId);

        if (flag) {
            value |= bitValue;
        } else {
            value &= ~bitValue;
        }

        qCInfo(lcHeadset) << "Sending 'mute' state with value" << flag << "to headset with usage"
                          << usage;
        send(usage.reportId, value);
    }
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

        qCInfo(lcHeadset) << "Sending 'ring' state with value" << flag
                          << "to headset (LED_Ring) with usage" << usage;

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

        qCInfo(lcHeadset) << "Sending 'ring' state with value" << flag
                          << "to headset (Telephony_Ringer) with usage" << usage;

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
    qCInfo(lcHeadset) << "Sending 'hold' state with value" << flag << "to headset with usage"
                      << usageHold;
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

void HeadsetDevice::setTeamsUsageMapping(QHash<UsageId, quint16> teamsUsageMapping)
{
    m_teamsUsageMapping = teamsUsageMapping;
}

void HeadsetDevice::processEvents()
{
    unsigned char data[64];
    memset(data, 0, sizeof(data));

    if (auto len = hid_read_timeout(m_device, data, sizeof(data), 10)) {
        quint8 reportId = data[0];

        if (len >= 2 && m_inputReportIds.contains(reportId)) {
            unsigned value = data[1];
            qCInfo(lcHeadset).noquote().nospace()
                    << "Found relevant report with report id 0x" << QString::number(reportId, 16);

            qCDebug(lcHeadset) << "Received report data" << QString::asprintf("0x%08X", value);

            // Hook switch
            if (m_hidUsages.contains(UsageId::Telephony_HookSwitch)) {
                if (GlobalInfo::instance().isWorkaroundActive(GlobalInfo::WorkaroundId::GOW_001)
                    && m_ignoreHookTimer.isActive()) {
                    m_ignoreHookTimer.stop();
                } else {
                    const auto &usage = m_hidUsages.value(UsageId::Telephony_HookSwitch);
                    if (usage.reportId == reportId) {
                        bool _hookSwitch = value & (1 << usage.bitPosition);
                        if (m_hookSwitch != _hookSwitch) {
                            m_hookSwitch = _hookSwitch;
                            emit hookSwitch();
                            qCDebug(lcHeadset) << "  Hook switch changed to" << m_hookSwitch;
                        }
                    }
                }
            }

            // Muted
            if (m_appSettings.value("generic/disableMutePropagation", false).toBool()) {
                qCDebug(lcHeadset) << "Mute propagation is disabled - skipping";
                return;
            } else {
                if (m_hidUsages.contains(UsageId::Telephony_PhoneMute)) {
                    const auto &usage = m_hidUsages.value(UsageId::Telephony_PhoneMute);
                    if (usage.reportId == reportId && (value & (1 << usage.bitPosition))) {
                        m_muted = !m_muted;
                        emit mute();

                        qCDebug(lcHeadset) << "  Muted changed to" << m_muted;
                        setMute(m_muted);
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
                            qCDebug(lcHeadset) << "  Line busy tone changed to" << m_line;
                        }
                    }
                }
            }

            // Flashing
            if (m_hidUsages.contains(UsageId::Telephony_Flash)) {
                const auto &usage = m_hidUsages.value(UsageId::Telephony_Flash);
                if (usage.reportId == reportId && (value & (1 << usage.bitPosition))) {
                    m_flash = !m_flash;
                    emit flash();
                    qCDebug(lcHeadset) << "  Flashing changed to" << m_flash;
                }
            }

            // Programmable button event - fired on secondary call to quit incoming call
            if (m_hidUsages.contains(UsageId::Telephony_ProgrammableButton)) {
                const auto &usage = m_hidUsages.value(UsageId::Telephony_ProgrammableButton);
                if (usage.reportId == reportId && (value & (1 << usage.bitPosition))) {
                    emit programmableButton();
                    qCDebug(lcHeadset) << "  Programmable button pressed";
                }
            }

            // Keypad
            if (m_hidUsages.contains(UsageId::Telephony_TelephonyKeyPad)) {
                static QString number = "";
                static quint16 lastValue = 0xFF;
                static QHash<quint8, QString> keyMap = {
                    { 1, "0" },  { 2, "1" },  { 3, "2" },  { 4, "3" },  { 5, "4" },  { 6, "5" },
                    { 7, "6" },  { 8, "7" },  { 9, "8" },  { 10, "9" }, { 11, "*" }, { 12, "#" },
                    { 13, "A" }, { 14, "B" }, { 15, "C" }, { 16, "D" }
                };

                const auto &usage = m_hidUsages.value(UsageId::Telephony_TelephonyKeyPad);
                if (usage.reportId == reportId && len > 2) {
                    quint8 toDrop = usage.bitPosition / 8;
                    quint8 toShift = usage.bitPosition % 8;

                    if (toDrop + 2 <= len && usage.size <= 8) {

                        // Skip first reportId and toDrop bytes
                        quint16 v = data[toDrop + 1] + (data[toDrop + 2] << 8);
                        value = (v >> toShift) & ((1 << usage.size) - 1);

                        if (value == 0 && lastValue == 0) {
                            if (!number.isEmpty()) {
                                qCDebug(lcHeadset) << "  dial" << number;
                                emit dial(number);
                                number.clear();
                            }

                            lastValue = 0xFF;
                        } else if (value != 0) {
                            number.push_back(keyMap.value(value, ""));
                        }

                        lastValue = value;
                    }
                }
            }
        }

        // Teams button
        if (reportId == 0x9B && data[1] == 1) {
            emit teamsButton();
        }
    }
}

bool HeadsetDevice::displayFieldSupported(ReportDescriptorEnums::TeamsDisplayFieldSupport field)
{
    quint8 zeroIndexBitNumber = (quint8)field - 1;
    return m_displayFieldSupportIndex & (1 << zeroIndexBitNumber);
}

void HeadsetDevice::setDisplayField(ReportDescriptorEnums::TeamsDisplayFieldSupport field,
                                    const QString &text)
{
    if (!m_device) {
        qCCritical(lcHeadset) << "device not open during attempt to send data";
        return;
    }

    if (displayFieldSupported(field)
        && m_teamsUsageMapping.contains(UsageId::Teams_CharacterAttributes)
        && m_teamsUsageMapping.contains(UsageId::Teams_CharacterReport)) {

        unsigned char buf[18];
        buf[0] = m_teamsUsageMapping[UsageId::Teams_CharacterAttributes];
        buf[1] = (quint8)field;
        buf[2] = text.length() ? 0x80 : 0;
        hid_write(m_device, buf, 3);

        auto chunks = makeChunks(text, 8);
        for (auto i = 0; i < chunks.length(); i++) {
            memset(buf, 0, sizeof(buf));
            buf[0] = m_teamsUsageMapping[UsageId::Teams_CharacterReport];
            buf[1] = i == chunks.length() - 1 ? 0x80 : 0;

            QString chunk = chunks[i];
            auto utf16Chunk = chunk.utf16();
            qsizetype dataIndex = 2;

            for (auto c = 0; c < chunk.length(); c++) {
                ushort ch = utf16Chunk[c];
                buf[dataIndex++] = ch & 0x00FF;
                buf[dataIndex++] = ch >> 8;
            }

            hid_write(m_device, buf, sizeof(buf));
        }
    }
}

QStringList HeadsetDevice::makeChunks(const QString &text, qsizetype chunkSize)
{
    QStringList res;

    qsizetype count = text.length() / chunkSize;
    qsizetype remainder = text.length() % chunkSize;

    for (auto c = 0; c < count; c++) {
        res.push_back(text.mid(c * chunkSize, chunkSize));
    }

    if (remainder) {
        res.push_back(text.right(remainder));
    }

    return res;
}

void HeadsetDevice::syncDateAndTime()
{
    QLocale locale;

    auto now = QDateTime::currentDateTime();
    now.toString();

    setDisplayField(ReportDescriptorEnums::TeamsDisplayFieldSupport::Date,
                    locale.toString(now, tr("MMMM dd")));
    setDisplayField(ReportDescriptorEnums::TeamsDisplayFieldSupport::Time,
                    now.toString("h:mm ap").toUpper());
}

void HeadsetDevice::setLocalUserName(const QString &name)
{
    setDisplayField(ReportDescriptorEnums::TeamsDisplayFieldSupport::LocalUserName, name);
}

void HeadsetDevice::setLocalUserNumber(const QString &number)
{
    setDisplayField(ReportDescriptorEnums::TeamsDisplayFieldSupport::Number, number);
}

void HeadsetDevice::setLocalUserStatus(const QString &status)
{
    setDisplayField(ReportDescriptorEnums::TeamsDisplayFieldSupport::LocalUserStatus, status);
}

void HeadsetDevice::setOtherUserName(const QString &name)
{
    setDisplayField(ReportDescriptorEnums::TeamsDisplayFieldSupport::OtherPartyName, name);
}

void HeadsetDevice::setOtherUserNumber(const QString &number)
{
    setDisplayField(ReportDescriptorEnums::TeamsDisplayFieldSupport::OtherPartyNumber, number);
}

void HeadsetDevice::setSubject(const QString &subject)
{
    setDisplayField(ReportDescriptorEnums::TeamsDisplayFieldSupport::Subject, subject);
}

void HeadsetDevice::setCallStatus(const QString &state)
{
    setDisplayField(ReportDescriptorEnums::TeamsDisplayFieldSupport::CallStatus, state);
}

void HeadsetDevice::selectScreen(ReportDescriptorEnums::TeamsScreenSelect screen, bool clear,
                                 bool backlight)
{
    if (!m_device) {
        qCCritical(lcHeadset) << "device not open during attempt to send data";
        return;
    }

    if (m_teamsUsageMapping.contains(UsageId::Teams_DisplayControl)) {
        unsigned char buf[2];
        buf[0] = m_teamsUsageMapping[UsageId::Teams_DisplayControl];
        buf[1] = (((quint8)screen & 0b1111) << 3) + (backlight ? 4 : 0) + (clear ? 2 : 0)
                + 1; // 1 for enable
        hid_write(m_device, buf, sizeof(buf));
    }
}

void HeadsetDevice::setPresenceIcon(ReportDescriptorEnums::TeamsPresenceIcon icon)
{
    if (!m_device) {
        qCCritical(lcHeadset) << "device not open during attempt to send data";
        return;
    }

    m_presenceIcon = icon;

    if (m_teamsUsageMapping.contains(UsageId::Teams_IconsControl)) {
        unsigned char buf[3];
        buf[0] = m_teamsUsageMapping[UsageId::Teams_IconsControl];
        buf[1] = (quint8)m_presenceIcon & 0b1111;
        buf[2] = 0;
        hid_write(m_device, buf, sizeof(buf));
    }
}

void HeadsetDevice::sendASP(quint8 cmd)
{
    if (!m_device) {
        qCCritical(lcHeadset) << "device not open during attempt to send data";
        return;
    }

    if (m_teamsUsageMapping.contains(UsageId::Teams_ASPNotification)) {
        unsigned char buf[64];
        memset(buf, 0, sizeof(buf));
        buf[0] = m_teamsUsageMapping[UsageId::Teams_ASPNotification];
        buf[1] = cmd; // 0x00, 0x10 and 0x40 have been captured
        hid_write(m_device, buf, sizeof(buf));
    }
}

UsageInfo::UsageInfo(qsizetype bitPosition, quint32 size, quint8 reportId)
{
    this->bitPosition = bitPosition;
    this->size = size;
    this->reportId = reportId;
}
