#include "GlobalCallState.h"
#include "Ringer.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcGlobalCallState, "gonnect.callstate")

GlobalCallState::GlobalCallState(QObject *parent) : QObject{ parent }
{
    connect(this, &GlobalCallState::globalCallStateChanged, this, &GlobalCallState::updateRinger);
    connect(this, &GlobalCallState::callInForegroundChanged, this,
            &GlobalCallState::onCallInForegroundChanged);
    connect(this, &GlobalCallState::callInForegroundChanged, this,
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
        disconnect(callStateObject);
        updateGlobalCallState();
        Q_EMIT globalCallStateObjectsChanged();
    }

    return wasRegistered;
}

void GlobalCallState::updateGlobalCallState()
{
    ICallState::States globalState = ICallState::State::Idle;
    for (const auto obj : std::as_const(m_globalCallStateObjects)) {
        auto state = obj->callState();
        globalState |= state;
    }

    setGlobalCallState(globalState);
    updateRemoteContactInfo();
    Q_EMIT activeCallsCountChanged();
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
        if (callObj->callState().toInt()) {
            ++count;
        }
    }
    return count;
}

void GlobalCallState::setCallInForeground(ICallState *call)
{
    if (m_callInForeground != call) {
        m_callInForeground = call;
        Q_EMIT callInForegroundChanged();
    }
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
    if (m_callInForeground) {
        if (m_callInForeground->callState() & ICallState::State::OnHold) {
            holdAllCalls(m_callInForeground);
            m_callInForeground->toggleHold();
        } else {
            holdAllCalls();
        }
    }
}

void GlobalCallState::holdAllCalls(const ICallState *stateObjectToSkip) const
{
    for (auto callObj : std::as_const(m_globalCallStateObjects)) {
        if (callObj != stateObjectToSkip && !(callObj->callState() & ICallState::State::OnHold)) {
            callObj->toggleHold();
        }
    }
}

void GlobalCallState::unholdOtherCall() const
{
    if (m_callInForeground) {
        if (m_callInForeground->callState() & ICallState::State::OnHold) {
            m_callInForeground->toggleHold();
        }
    } else {
        for (auto callObj : std::as_const(m_globalCallStateObjects)) {
            if (callObj->callState() & ICallState::State::OnHold) {
                callObj->toggleHold();
            }
            break;
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

void GlobalCallState::onCallInForegroundChanged()
{
    if (m_foregroundCallContext) {
        m_foregroundCallContext->deleteLater();
        m_foregroundCallContext = nullptr;
    }

    if (m_callInForeground) {
        connect(m_callInForeground, &QObject::destroyed, m_foregroundCallContext,
                [this]() { setProperty("callInForeground", QVariant::fromValue(nullptr)); });
    }
}

void GlobalCallState::updateRemoteContactInfo()
{
    using State = ICallState::State;
    using States = ICallState::States;

    ICallState *callObj = nullptr;

    const auto ringingCalls = filteredCallStateObjected(
            States::fromInt(0),
            State::RingingIncoming | State::RingingOutgoing | State::KnockingIncoming);
    const auto activeCalls = filteredCallStateObjected(State::CallActive);
    const auto activeCallsWithAudio =
            filteredCallStateObjected(State::CallActive | State::AudioActive);
    const auto activeCallsOnHold = filteredCallStateObjected(State::CallActive | State::OnHold);
    const auto reallyActiveCalls = activeCalls - activeCallsOnHold;

    if (!callObj && !ringingCalls.isEmpty()) {
        if (m_callInForeground && ringingCalls.contains(m_callInForeground)) {
            callObj = m_callInForeground;
        } else {
            callObj = *ringingCalls.constBegin();
        }
    }

    if (!callObj && !reallyActiveCalls.isEmpty()) {
        if (m_callInForeground && reallyActiveCalls.contains(m_callInForeground)) {
            callObj = m_callInForeground;
        } else {
            callObj = *reallyActiveCalls.constBegin();
        }
    }

    if (!callObj && !activeCallsWithAudio.isEmpty()) {
        if (m_callInForeground && activeCallsWithAudio.contains(m_callInForeground)) {
            callObj = m_callInForeground;
        } else {
            callObj = *activeCallsWithAudio.constBegin();
        }
    }

    if (!callObj && !activeCalls.isEmpty()) {
        if (m_callInForeground && activeCalls.contains(m_callInForeground)) {
            callObj = m_callInForeground;
        } else {
            callObj = *activeCalls.constBegin();
        }
    }

    const auto contactInfo = callObj ? callObj->remoteContactInfo() : ContactInfo();

    qCInfo(lcGlobalCallState) << "Updating global contact info to" << contactInfo;

    setRemoteContactInfo(contactInfo);
}
