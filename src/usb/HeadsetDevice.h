#pragma once
#include <hidapi.h>
#include <QObject>
#include <QTimer>
#include "IHeadsetDevice.h"
#include "ReportDescriptorStructs.h"

struct UsageInfo
{
    qsizetype bitPosition = 0;
    quint8 reportId = 0;
};

class HeadsetDevice final : public IHeadsetDevice
{
    Q_OBJECT

public:
    explicit HeadsetDevice(const hid_device_info *deviceInfo, QObject *parent = nullptr);

    QString name() const { return m_productName; }

    bool open();
    bool isOpen() const { return m_isOpen; }
    void close();

    void setBusyLine(bool flag);
    bool getBusyLine() const { return m_line; }

    bool getHookSwitch() const { return m_hookSwitch; }
    bool getFlash() const { return m_flash; }

    void setRing(bool flag);
    void setHold(bool flag);

    void setMute(bool flag);
    bool getMute() const { return m_muted; }

    void setIdle();

    void setUsageInfos(const QHash<UsageId, UsageInfo> &infos);

    ~HeadsetDevice();

private:
    void send(quint8 reportId, unsigned data);
    void processEvents();
    unsigned currentFlags(const quint32 reportId) const;

    QTimer m_eventHandler;
    QHash<UsageId, UsageInfo> m_hidUsages;

    QString m_path;
    QString m_productName;

    hid_device *m_device = nullptr;

    // TODO: features, etc.

    quint8 m_bus = 0;
    quint8 m_port = 0;
    QSet<quint8> m_inputReportIds;

    bool m_hookSwitch = false;
    bool m_line = false;
    bool m_muted = false;
    bool m_flash = false;
    bool m_hold = false;
    bool m_ringing = false;
    bool m_isOpen = false;
};
