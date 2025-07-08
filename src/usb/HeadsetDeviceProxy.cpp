#include <QDebug>
#include <QLoggingCategory>
#include <QUuid>
#include "UserInfo.h"
#include "USBDevices.h"
#include "HeadsetDevice.h"
#include "HeadsetDeviceProxy.h"
#include "GlobalCallState.h"
#include "GlobalMuteState.h"

Q_LOGGING_CATEGORY(lcHeadsetProxy, "gonnect.usb.headsetproxy")

using namespace std::chrono_literals;

HeadsetDeviceProxy::HeadsetDeviceProxy(QObject *parent) : IHeadsetDevice(parent)
{
    auto &devs = USBDevices::instance();
    connect(&devs, &USBDevices::devicesChanged, this, &HeadsetDeviceProxy::refreshDevice);

    refreshDevice();

    connect(&GlobalCallState::instance(), &GlobalCallState::globalCallStateChanged, this,
            &HeadsetDeviceProxy::updateDeviceState);

    connect(&GlobalMuteState::instance(), &GlobalMuteState::isMutedChangedWithTag, this,
            [this](const bool value, const QString tag) {
                if (m_muteTag.isEmpty() || m_muteTag != tag) {
                    setMute(value);
                } else if (!m_muteTag.isEmpty() && m_muteTag == tag) {
                    // We started the change request so ignore propagation
                    m_muteTag.clear();
                }
            });

    connect(&GlobalCallState::instance(), &GlobalCallState::remoteContactInfoChanged, this,
            &HeadsetDeviceProxy::updateRemoteContactInfo);
    connect(&GlobalCallState::instance(), &GlobalCallState::isPhoneConferenceChanged, this,
            &HeadsetDeviceProxy::updateRemoteContactInfo);
}

HeadsetDeviceProxy::~HeadsetDeviceProxy()
{
    close();
}

void HeadsetDeviceProxy::updateDeviceState()
{
    if (!m_device) {
        return;
    }

    typedef ICallState::State State;

    const auto state = GlobalCallState::instance().globalCallState();
    const auto changeMask = m_oldCallState ^ state;

    m_oldCallState = state;
    if (changeMask & State::RingingIncoming) {
        setRing(state & State::RingingIncoming);
        if (state & State::RingingIncoming) {
            m_inRemoteCallScreen = true;
            m_device->setCallStatus(tr("Ringing"));
            m_device->selectScreen(ReportDescriptorEnums::TeamsScreenSelect::IncomingCall);
        }
    }

    if (changeMask & State::KnockingIncoming) {
        setRing(state & State::KnockingIncoming);
        if (state & State::KnockingIncoming) {
            m_inRemoteCallScreen = true;
            m_device->setCallStatus(tr("Call waiting"));
            m_device->selectScreen(ReportDescriptorEnums::TeamsScreenSelect::IncomingCall);
        } else if (state & State::CallActive) {
            m_inRemoteCallScreen = true;
            m_device->setCallStatus(tr("Call active"));
            m_device->selectScreen(ReportDescriptorEnums::TeamsScreenSelect::InCall);
        }
    }

    if (changeMask & State::RingingOutgoing) {
        if (state & State::RingingOutgoing) {
            m_inRemoteCallScreen = true;
            m_device->setCallStatus(tr("Calling"));
            m_device->selectScreen(ReportDescriptorEnums::TeamsScreenSelect::OutgoingCall);
        } else if (state & State::CallActive) {
            m_inRemoteCallScreen = true;
            m_device->setCallStatus(tr("Call active"));
            m_device->selectScreen(ReportDescriptorEnums::TeamsScreenSelect::InCall);
        }
    }

    if (changeMask & State::OnHold) {
        setHold(state & State::OnHold);
        if (state & State::OnHold) {
            m_inRemoteCallScreen = true;
            m_device->setCallStatus(tr("On Hold"));
            m_device->selectScreen(ReportDescriptorEnums::TeamsScreenSelect::HoldCall);
        } else {
            m_inRemoteCallScreen = true;
            m_device->setCallStatus(tr("Call active"));
            m_device->selectScreen(ReportDescriptorEnums::TeamsScreenSelect::InCall);
        }
    }

    // IDLE
    if (changeMask && !state) {
        setIdle();
        m_inRemoteCallScreen = false;
        m_device->setCallStatus(tr("Call ended"));
        m_device->selectScreen(ReportDescriptorEnums::TeamsScreenSelect::HomeScreen);
    }

    if (changeMask & State::AudioActive && !(state & State::Migrating)) {
        GlobalMuteState::instance().reset();
        setBusyLine(state & State::AudioActive);
    }

    if (changeMask & State::CallActive) {
        if (state & State::CallActive) {
            m_inRemoteCallScreen = true;
            m_device->setCallStatus(tr("Call active"));
            m_device->selectScreen(ReportDescriptorEnums::TeamsScreenSelect::InCall);
        } else {
            m_inRemoteCallScreen = false;
            m_device->setCallStatus(tr("Call ended"));
            m_device->selectScreen(ReportDescriptorEnums::TeamsScreenSelect::EndCall);
        }
    }

    updateRemoteContactInfo();
}

void HeadsetDeviceProxy::updateRemoteContactInfo()
{
    if (!m_device) {
        return;
    }

    if (m_inRemoteCallScreen) {
        auto rci = GlobalCallState::instance().remoteContactInfo();
        bool isPhoneConference = GlobalCallState::instance().isPhoneConference();

        m_device->setOtherUserName(isPhoneConference ? tr("Phone conference") : rci.displayName);
        m_device->setOtherUserNumber(rci.phoneNumber);
    }
}

bool HeadsetDeviceProxy::refreshDevice()
{
    auto devs = USBDevices::instance().headsetDevices();
    if (m_device) {
        m_device = nullptr;
    }

    if (devs.count()) {
        m_device = devs.first();

        connect(m_device, &HeadsetDevice::hookSwitch, this, [this]() {
            if (isEnabled()) {
                emit hookSwitch();
            }
        });
        connect(m_device, &HeadsetDevice::hookSwitchTrigger, this, [this]() {
            if (isEnabled()) {
                emit hookSwitchTrigger();
            }
        });
        connect(m_device, &HeadsetDevice::mute, this, [this]() {
            if (isEnabled()) {
                emit mute();
                m_muteTag = QUuid::createUuid().toString();
                GlobalMuteState::instance().toggleMute(m_muteTag);
            }
        });
        connect(m_device, &HeadsetDevice::busyLine, this, [this]() {
            if (isEnabled()) {
                emit busyLine();
            }
        });
        connect(m_device, &HeadsetDevice::flash, this, [this]() {
            if (isEnabled()) {
                emit flash();
                GlobalCallState::instance().triggerHold();
            }
        });
        connect(m_device, &HeadsetDevice::teamsButton, this, [this]() {
            if (isEnabled()) {
                emit teamsButton();
            }
        });
        connect(m_device, &HeadsetDevice::programmableButton, this, [this]() {
            if (isEnabled()) {
                emit programmableButton();
            }
        });
        connect(m_device, &HeadsetDevice::dial, this, [this](const QString &number) {
            if (isEnabled()) {
                emit dial(number);
            }
        });
        open();

        QTimer::singleShot(500ms, this, [this]() { updateDeviceState(); });
    }

    emit nameChanged();

    return !!m_device;
}

QString HeadsetDeviceProxy::name() const
{
    if (m_device) {
        return m_device->name();
    }

    return "";
}

bool HeadsetDeviceProxy::getBusyLine() const
{
    if (m_device) {
        return m_device->getBusyLine();
    }

    return false;
}

bool HeadsetDeviceProxy::getHookSwitch() const
{
    if (m_device) {
        return m_device->getHookSwitch();
    }

    return false;
}

bool HeadsetDeviceProxy::getFlash() const
{
    if (m_device) {
        return m_device->getFlash();
    }

    return false;
}

bool HeadsetDeviceProxy::open()
{
    if (m_device && !m_device->isOpen()) {

        if (m_device->open()) {
            qCInfo(lcHeadsetProxy) << "Using USB headset:" << HeadsetDeviceProxy::name();
            m_device->setIdle();
            m_device->syncDateAndTime();
            m_device->selectScreen(ReportDescriptorEnums::TeamsScreenSelect::HomeScreen);

            auto &ui = UserInfo::instance();
            QString displayName = ui.getDisplayName();

            if (displayName.isEmpty()) {
                connect(
                        &ui, &UserInfo::displayNameChanged, this,
                        [this]() {
                            m_device->setLocalUserName(UserInfo::instance().getDisplayName());
                            m_device->setPresenceIcon(
                                    ReportDescriptorEnums::TeamsPresenceIcon::Online);
                        },
                        Qt::SingleShotConnection);
            } else {
                m_device->setLocalUserName(displayName);
                m_device->setPresenceIcon(ReportDescriptorEnums::TeamsPresenceIcon::Online);
            }

            return true;
        }
    }

    return false;
}

void HeadsetDeviceProxy::close()
{
    if (m_device) {
        m_device->close();
    }
}

void HeadsetDeviceProxy::setIdle()
{
    if (m_device) {
        m_device->setIdle();
    }
}

void HeadsetDeviceProxy::syncDateAndTime()
{
    if (m_device) {
        m_device->syncDateAndTime();
    }
}

void HeadsetDeviceProxy::setLocalUserName(const QString &name)
{
    if (m_device) {
        m_device->setLocalUserName(name);
    }
}

void HeadsetDeviceProxy::setLocalUserNumber(const QString &number)
{
    if (m_device) {
        m_device->setLocalUserNumber(number);
    }
}

void HeadsetDeviceProxy::setLocalUserStatus(const QString &status)
{
    if (m_device) {
        m_device->setLocalUserStatus(status);
    }
}

void HeadsetDeviceProxy::setOtherUserName(const QString &name)
{
    if (m_device) {
        m_device->setOtherUserName(name);
    }
}

void HeadsetDeviceProxy::setOtherUserNumber(const QString &number)
{
    if (m_device) {
        m_device->setOtherUserNumber(number);
    }
}

void HeadsetDeviceProxy::setSubject(const QString &subject)
{
    if (m_device) {
        m_device->setSubject(subject);
    }
}

void HeadsetDeviceProxy::selectScreen(ReportDescriptorEnums::TeamsScreenSelect screen, bool clear,
                                      bool backlight)
{
    if (m_device) {
        m_device->selectScreen(screen, clear, backlight);
    }
}

void HeadsetDeviceProxy::setPresenceIcon(ReportDescriptorEnums::TeamsPresenceIcon icon)
{
    if (m_device) {
        m_device->setPresenceIcon(icon);
    }
}

void HeadsetDeviceProxy::setCallStatus(const QString &state)
{
    if (m_device) {
        m_device->setCallStatus(state);
    }
}

void HeadsetDeviceProxy::setBusyLine(bool flag)
{
    if (isEnabled() && m_device) {
        m_device->setBusyLine(flag);
    }
}

void HeadsetDeviceProxy::setMute(bool flag)
{
    if (isEnabled() && m_device) {
        m_device->setMute(flag);
    }
}

void HeadsetDeviceProxy::setRing(bool flag)
{
    if (isEnabled() && m_device) {
        m_device->setRing(flag);
    }
}

void HeadsetDeviceProxy::setHold(bool flag)
{
    if (isEnabled() && m_device) {
        m_device->setHold(flag);
    }
}
