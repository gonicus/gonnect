#include "GlobalCallState.h"
#include "SelectionState.h"
#include "Ringer.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcGlobalCallState, "gonnect.callstate")

GlobalCallState::GlobalCallState(QObject *parent) : QObject{ parent }
{
    connect(this, &GlobalCallState::globalCallStateChanged, this, &GlobalCallState::updateRinger);
    connect(&SelectionState::instance(), &SelectionState::callInForegroundChanged, this,
            &GlobalCallState::updateRemoteContactInfo);
}

void GlobalCallState::setGlobalCallState(const ICallState::States state)
{
    if (m_globalCallState != state) {
        m_globalCallState = state;
        Q_EMIT globalCallStateChanged();

        // Debug output
        qCInfo(lcGlobalCallState).noquote().nospace()
                << "Global call state changed to: "
                << ICallState::statesAsStrings(m_globalCallState).join(", ");
    }
}

QSet<ICallState *>
GlobalCallState::filteredCallStateObjected(const ICallState::States mustFulfilAll,
                                           const ICallState::States mustFulfilOne) const
{
    QSet<ICallState *> result;

    for (auto stateObj : std::as_const(m_globalCallStateObjects)) {
        if ((!mustFulfilAll.toInt() || ((stateObj->callState() & mustFulfilAll) == mustFulfilAll))
            && (!mustFulfilOne.toInt() || (stateObj->callState() & mustFulfilOne).toInt())) {
            result.insert(stateObj);
        }
    }

    return result;
}

bool GlobalCallState::registerCallStateObject(ICallState *callStateObject)
{
    Q_CHECK_PTR(callStateObject);

    const bool isRegistered = m_globalCallStateObjects.contains(callStateObject);
    if (!isRegistered) {
        m_globalCallStateObjects.insert(callStateObject);

        connect(callStateObject, &QObject::destroyed, this, [this](QObject *obj) {
            if (m_globalCallStateObjects.remove(static_cast<ICallState *>(obj))) {
                updateGlobalCallState();
                Q_EMIT globalCallStateObjectsChanged();
            }
        });

        connect(callStateObject, &ICallState::callStateChanged, this,
                &GlobalCallState::updateGlobalCallState);

        updateGlobalCallState();
        Q_EMIT globalCallStateObjectsChanged();
    }

    return !isRegistered;
}

bool GlobalCallState::unregisterCallStateObject(ICallState *callStateObject)
{
    if (!callStateObject) {
        return false;
    }

    const bool wasRegistered = m_globalCallStateObjects.remove(callStateObject);
    if (wasRegistered) {
        disconnect(callStateObject, nullptr, this, nullptr);
        updateGlobalCallState();
        Q_EMIT globalCallStateObjectsChanged();
    }

    return wasRegistered;
}

void GlobalCallState::updateGlobalCallState()
{
    ICallState::States globalState = ICallState::State::Idle;
    bool hasNonHoldCall = false;

    for (const auto obj : std::as_const(m_globalCallStateObjects)) {
        auto state = obj->callState();
        globalState |= state;

        if ((state & ICallState::State::CallActive) && !(state & ICallState::State::OnHold)) {
            hasNonHoldCall = true;
        }
    }

    if (hasNonHoldCall) {
        globalState.setFlag(ICallState::State::OnHold, false);
    }

    setGlobalCallState(globalState);

    updateRemoteContactInfo();
    Q_EMIT activeCallsCountChanged();
    Q_EMIT nonIdleCallsCountChanged();
}

void GlobalCallState::setIsPhoneConference(bool flag)
{
    if (flag != m_isPhoneConference) {
        m_isPhoneConference = flag;
        Q_EMIT isPhoneConferenceChanged();
    }
}

qsizetype GlobalCallState::activeCallsCount() const
{
    qsizetype count = 0;
    for (const auto callObj : std::as_const(m_globalCallStateObjects)) {
        if (callObj->callState() & ICallState::State::CallActive) {
            ++count;
        }
    }
    return count;
}

qsizetype GlobalCallState::nonIdleCallsCount() const
{
    qsizetype count = 0;
    for (const auto callObj : std::as_const(m_globalCallStateObjects)) {
        if (callObj->callState().toInt()) {
            ++count;
        }
    }
    return count;
}

void GlobalCallState::setRemoteContactInfo(const ContactInfo &info)
{
    if (m_remoteContactInfo != info) {
        m_remoteContactInfo = info;
        Q_EMIT remoteContactInfoChanged();
    }
}

void GlobalCallState::triggerHold()
{
    auto *callInForeground = SelectionState::instance().callInForeground();
    if (callInForeground) {
        if (callInForeground->callState() & ICallState::State::OnHold) {
            holdAllCalls(callInForeground);
            callInForeground->toggleHold();
        } else {
            holdAllCalls();
        }
    }
}

void GlobalCallState::holdAllCalls(const ICallState *stateObjectToSkip) const
{
    for (auto callObj : std::as_const(m_globalCallStateObjects)) {
        if (callObj != stateObjectToSkip && (callObj->callState() & ICallState::State::CallActive)
            && !(callObj->callState() & ICallState::State::OnHold)) {
            callObj->toggleHold();
        }
    }
}

void GlobalCallState::unholdOtherCall() const
{
    auto *callInForeground = SelectionState::instance().callInForeground();

    if (callInForeground) {
        if (callInForeground->callState() & ICallState::State::OnHold) {
            callInForeground->toggleHold();
        }
    } else {
        for (auto callObj : std::as_const(m_globalCallStateObjects)) {
            if (callObj->callState() & ICallState::State::OnHold) {
                callObj->toggleHold();
                break;
            }
        }
    }
}

void GlobalCallState::unholdAllCalls() const
{
    for (auto callObj : std::as_const(m_globalCallStateObjects)) {
        if (callObj->callState() & ICallState::State::OnHold) {
            callObj->toggleHold();
        }
    }
}

void GlobalCallState::updateRinger()
{
    const bool isRinging = m_globalCallState & ICallState::State::RingingIncoming;

    if (isRinging && !m_ringer) {
        m_ringer = new Ringer(this);
        m_ringer->start();

    } else if (m_ringer && !isRinging) {
        m_ringer->deleteLater();
        m_ringer = nullptr;
    }
}

void GlobalCallState::updateRemoteContactInfo()
{
    using State = ICallState::State;
    using States = ICallState::States;

    ICallState *callObj = nullptr;

    auto *callInForeground = SelectionState::instance().callInForeground();

    const auto ringingCalls = filteredCallStateObjected(
            States::fromInt(0),
            State::RingingIncoming | State::RingingOutgoing | State::KnockingIncoming);
    const auto activeCalls = filteredCallStateObjected(State::CallActive);
    const auto activeCallsWithAudio =
            filteredCallStateObjected(State::CallActive | State::AudioActive);
    const auto activeCallsOnHold = filteredCallStateObjected(State::CallActive | State::OnHold);
    const auto reallyActiveCalls = activeCalls - activeCallsOnHold;

    if (!callObj && !ringingCalls.isEmpty()) {
        if (callInForeground && ringingCalls.contains(callInForeground)) {
            callObj = callInForeground;
        } else {
            callObj = *ringingCalls.constBegin();
        }
    }

    if (!callObj && !reallyActiveCalls.isEmpty()) {
        if (callInForeground && reallyActiveCalls.contains(callInForeground)) {
            callObj = callInForeground;
        } else {
            callObj = *reallyActiveCalls.constBegin();
        }
    }

    if (!callObj && !activeCallsWithAudio.isEmpty()) {
        if (callInForeground && activeCallsWithAudio.contains(callInForeground)) {
            callObj = callInForeground;
        } else {
            callObj = *activeCallsWithAudio.constBegin();
        }
    }

    if (!callObj && !activeCalls.isEmpty()) {
        if (callInForeground && activeCalls.contains(callInForeground)) {
            callObj = callInForeground;
        } else {
            callObj = *activeCalls.constBegin();
        }
    }

    const auto contactInfo = callObj ? callObj->remoteContactInfo() : ContactInfo();

    qCInfo(lcGlobalCallState) << "Updating global contact info to" << contactInfo;

    setRemoteContactInfo(contactInfo);
}
