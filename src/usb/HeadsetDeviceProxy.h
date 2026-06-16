#pragma once
#include <hidapi.h>
#include <QObject>
#include <QTimer>
#include <QtQml/qqml.h>
#include <QtQml/qqmlregistration.h>
#include "IHeadsetDevice.h"
#include "ICallState.h"
#include "AppSettings.h"
#include "MuteSyncGuard.h"

class HeadsetDevice;

class HeadsetDeviceProxy : public IHeadsetDevice
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(bool muteLocked READ muteLocked NOTIFY muteLockedChanged)

public:
    explicit HeadsetDeviceProxy(QObject *parent = nullptr);

    virtual QString name() const;

    virtual bool open() final;
    virtual void close() final;

    virtual void setBusyLine(bool flag);
    virtual bool getBusyLine() const;

    virtual bool getHookSwitch() const;
    virtual bool getFlash() const;

    virtual void setRing(bool flag);
    virtual void setHold(bool flag);

    virtual void setIdle();

    virtual void syncDateAndTime();
    virtual void setLocalUserName(const QString &name);
    virtual void setLocalUserNumber(const QString &number);
    virtual void setLocalUserStatus(const QString &status);
    virtual void setOtherUserName(const QString &name);
    virtual void setOtherUserNumber(const QString &number);
    virtual void setSubject(const QString &subject);
    virtual void selectScreen(ReportDescriptorEnums::TeamsScreenSelect screen, bool clear = false,
                              bool backlight = true);
    virtual void setPresenceIcon(ReportDescriptorEnums::TeamsPresenceIcon icon);
    virtual void setCallStatus(const QString &state);

    bool available() const { return !!m_device; }
    bool muteLocked() const { return m_muteLocked; }
    void setMute(bool flag);

    ~HeadsetDeviceProxy();

Q_SIGNALS:
    void nameChanged();
    void muteLockedChanged();

protected Q_SLOTS:
    void updateDeviceState(bool refreshAll = false);
    void updateRemoteContactInfo();

private:
    bool refreshDevice();
    bool isEnabled() { return m_settings.value("generic/useHeadset", true).toBool(); }
    void setMuteLocked(bool locked);

    AppSettings m_settings;
    HeadsetDevice *m_device = nullptr;
    ICallState::States m_oldCallState = ICallState::State::Idle;

    QTimer m_callStartTimer;
    QTimer m_callEndTimer;

    bool m_inRemoteCallScreen = false;
    bool m_muteLocked = false;
    MuteSyncGuard m_muteSync;
};

class HeadsetDeviceProxyWrapper
{
    Q_GADGET
    QML_FOREIGN(HeadsetDeviceProxy)
    QML_NAMED_ELEMENT(HeadsetDeviceProxy)
    QML_UNCREATABLE("object is owned by the main application")
};
