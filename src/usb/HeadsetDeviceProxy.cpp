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
            [this]() { updateDeviceState(); });

    connect(&GlobalMuteState::instance(), &GlobalMuteState::isMutedChangedWithTag, this,
            [this](const bool value, const QString tag) {
                // Ignore the echo of our own origination, otherwise re-apply
                if (!m_muteSync.isOwnEcho(tag)) {
                    setMute(value);
                }
            });

    connect(&GlobalCallState::instance(), &GlobalCallState::remoteContactInfoChanged, this,
            &HeadsetDeviceProxy::updateRemoteContactInfo);
    connect(&GlobalCallState::instance(), &GlobalCallState::isPhoneConferenceChanged, this,
            &HeadsetDeviceProxy::updateRemoteContactInfo);

    m_callEndTimer.setInterval(2s);
    m_callEndTimer.setSingleShot(true);
    connect(&m_callEndTimer, &QTimer::timeout, this, [this]() {
        if (!m_device) {
            return;
        }

        if (GlobalCallState::instance().globalCallState() == ICallState::State::Idle) {
            m_device->setCallStatus(tr("Call ended"));
            m_device->selectScreen(ReportDescriptorEnums::TeamsScreenSelect::HomeScreen);
        }
    });

    m_callStartTimer.setInterval(500ms);
    m_callStartTimer.setSingleShot(true);
    connect(&m_callStartTimer, &QTimer::timeout, this, [this]() {
        if (!m_device) {
            return;
        }

        const auto state = GlobalCallState::instance().globalCallState();
        if ((state & ICallState::State::CallActive) && !(state & ICallState::State::OnHold)) {
            m_inRemoteCallScreen = true;
            m_device->setCallStatus(tr("Call active"));
            m_device->selectScreen(ReportDescriptorEnums::TeamsScreenSelect::InCall);
            updateRemoteContactInfo();
        }
    });
}

HeadsetDeviceProxy::~HeadsetDeviceProxy()
{
    close();
}

void HeadsetDeviceProxy::updateDeviceState(bool refreshAll)
{
    if (!m_device) {
        return;
    }

    using State = ICallState::State;

    const auto state = GlobalCallState::instance().globalCallState();
    const auto changeMask =
            refreshAll ? ICallState::States::fromInt((1 << 9) - 1) : (m_oldCallState ^ state);

    m_oldCallState = state;

    if (state && m_callEndTimer.isActive()) {
        m_callEndTimer.stop();
    }

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
            m_callStartTimer.start();
        }
    }

    if (changeMask & State::RingingOutgoing) {
        if (state & State::RingingOutgoing) {
            m_inRemoteCallScreen = true;
            m_device->setCallStatus(tr("Calling"));
            m_device->selectScreen(ReportDescriptorEnums::TeamsScreenSelect::OutgoingCall);
        } else if (state & State::CallActive) {
            m_inRemoteCallScreen = true;
            m_callStartTimer.start();
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
            m_callStartTimer.start();
        }
    }

    // IDLE
    if (changeMask && !state) {
        setIdle();
        m_callStartTimer.stop();
        m_inRemoteCallScreen = false;
        m_callEndTimer.start();
    }

    if (changeMask & State::AudioActive && !(state & State::Migrating)) {
        GlobalMuteState::instance().reset();
        if (state & State::AudioActive) {
            setBusyLine(true);
            if (m_device && !GlobalMuteState::instance().isMuted()) {
                m_device->probeMuteLock();
            }
        } else {
            setBusyLine(false);
        }
    }

    if (changeMask & State::CallActive) {
        if (state & State::CallActive) {
            m_inRemoteCallScreen = true;
            m_callStartTimer.start();
        }
    } else if (state) {
        m_inRemoteCallScreen = true;

        if (state & State::OnHold) {
            m_callStartTimer.stop();
            m_device->setCallStatus(tr("On Hold"));
            m_device->selectScreen(ReportDescriptorEnums::TeamsScreenSelect::HoldCall);
        } else if (state & (State::RingingIncoming | State::KnockingIncoming)) {
            m_callStartTimer.stop();
            m_device->setCallStatus(state & State::RingingIncoming ? tr("Ringing")
                                                                   : tr("Call waiting"));
            m_device->selectScreen(ReportDescriptorEnums::TeamsScreenSelect::IncomingCall);
        } else if (state & State::RingingOutgoing) {
            m_callStartTimer.stop();
            m_device->setCallStatus(tr("Calling"));
            m_device->selectScreen(ReportDescriptorEnums::TeamsScreenSelect::OutgoingCall);
        } else if (state & State::CallActive) {
            if (!m_callStartTimer.isActive()) {
                m_callStartTimer.start();
            }
        }
    }

    if (!m_callStartTimer.isActive()) {
        updateRemoteContactInfo();
    }
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

void HeadsetDeviceProxy::setMuteLocked(bool locked)
{
    if (m_muteLocked != locked) {
        m_muteLocked = locked;
        Q_EMIT muteLockedChanged();
    }
}

bool HeadsetDeviceProxy::refreshDevice()
{
    auto devs = USBDevices::instance().headsetDevices();
    if (m_device) {
        disconnect(m_device, nullptr, this, nullptr);
        m_device = nullptr;
    }

    // A reattached or removed device carries no lock state
    setMuteLocked(false);

    if (devs.count()) {
        m_device = devs.first();

        connect(m_device, &HeadsetDevice::destroyed, this, [this]() { m_device = nullptr; });

        connect(m_device, &HeadsetDevice::hookSwitch, this, [this]() {
            if (isEnabled()) {
                Q_EMIT hookSwitch();
            }
        });
        connect(m_device, &HeadsetDevice::mute, this, [this]() {
            if (isEnabled()) {
                Q_EMIT mute();
                GlobalMuteState::instance().setMuted(m_device->getMute(), m_muteSync.originate());
            }
        });
        connect(m_device, &HeadsetDevice::muteLockChanged, this,
                [this](bool locked, bool muted) {
                    if (isEnabled()) {
                        qCInfo(lcHeadsetProxy)
                                << "Headset mute-lock changed - locked:" << locked
                                << "muted:" << muted;
                        if (locked) {
                            GlobalMuteState::instance().setMuted(muted, m_muteSync.originate());
                        }
                        setMuteLocked(locked);
                    }
                });
        connect(m_device, &HeadsetDevice::busyLine, this, [this]() {
            if (isEnabled()) {
                Q_EMIT busyLine();
            }
        });
        connect(m_device, &HeadsetDevice::flash, this, [this]() {
            if (isEnabled()) {
                GlobalCallState::instance().triggerHold();
            }
        });
        connect(m_device, &HeadsetDevice::teamsButton, this, [this]() {
            if (isEnabled()) {
                Q_EMIT teamsButton();
            }
        });
        connect(m_device, &HeadsetDevice::programmableButton, this, [this]() {
            if (isEnabled()) {
                Q_EMIT programmableButton();
            }
        });
        connect(m_device, &HeadsetDevice::dial, this, [this](const QString &number) {
            if (isEnabled()) {
                Q_EMIT dial(number);
            }
        });
        open();

        QTimer::singleShot(500ms, this, [this]() { updateDeviceState(); });
    }

    Q_EMIT nameChanged();

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
                            if (m_device) {
                                m_device->setLocalUserName(UserInfo::instance().getDisplayName());
                                m_device->setPresenceIcon(
                                        ReportDescriptorEnums::TeamsPresenceIcon::Online);
                            }
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
