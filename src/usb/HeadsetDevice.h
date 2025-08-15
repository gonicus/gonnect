#pragma once
#include <hidapi.h>
#include <QObject>
#include <QTimer>
#include "AppSettings.h"
#include "IHeadsetDevice.h"
#include "ReportDescriptorStructs.h"

struct UsageInfo
{
    UsageInfo() { }
    UsageInfo(qsizetype bitPosition, quint32 size, quint8 reportId);

    qsizetype bitPosition = 0;
    quint32 size = 0;
    quint8 reportId = 0;
};

QDebug operator<<(QDebug debug, const UsageInfo &usageInfo);

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

    void syncDateAndTime();
    void setLocalUserName(const QString &name);
    void setLocalUserNumber(const QString &number);
    void setLocalUserStatus(const QString &status);
    void setOtherUserName(const QString &name);
    void setOtherUserNumber(const QString &number);
    void setSubject(const QString &subject);
    void selectScreen(ReportDescriptorEnums::TeamsScreenSelect screen, bool clear = false,
                      bool backlight = true);
    void setPresenceIcon(ReportDescriptorEnums::TeamsPresenceIcon icon);
    void setCallStatus(const QString &state);

    void setUsageInfos(const QHash<UsageId, UsageInfo> &infos);
    void setTeamsUsageMapping(QHash<UsageId, quint16> teamsUsageMapping);

    ~HeadsetDevice();

private:
    bool displayFieldSupported(ReportDescriptorEnums::TeamsDisplayFieldSupport field);
    void setDisplayField(ReportDescriptorEnums::TeamsDisplayFieldSupport field,
                         const QString &text);
    void sendASP(quint8 cmd);

    void send(quint8 reportId, unsigned data);
    void processEvents();
    unsigned currentFlags(const quint32 reportId) const;
    bool useHeadset() { return m_appSettings.value("generic/useHeadset", true).toBool(); }

    QStringList makeChunks(const QString &text, qsizetype chunkSize);

    AppSettings m_appSettings;
    QTimer m_eventHandler;
    QTimer m_ignoreHookTimer;
    QHash<UsageId, UsageInfo> m_hidUsages;
    QHash<UsageId, quint16> m_teamsUsageMapping;

    QString m_path;
    QString m_productName;

    hid_device *m_device = nullptr;

    quint32 m_displayRows = 0;
    quint32 m_displayCols = 0;
    quint32 m_displayFieldSupportIndex = 0;

    quint8 m_bus = 0;
    quint8 m_port = 0;
    QSet<quint8> m_inputReportIds;

    ReportDescriptorEnums::TeamsPresenceIcon m_presenceIcon =
            ReportDescriptorEnums::TeamsPresenceIcon::Offline;

    bool m_hookSwitch = false;
    bool m_line = false;
    bool m_muted = false;
    bool m_flash = false;
    bool m_hold = false;
    bool m_ringing = false;
    bool m_isOpen = false;

    bool m_ignoreNextMuteUpdate = false;
    bool m_displaySupported = false;
};
