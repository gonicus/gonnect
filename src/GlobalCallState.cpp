#include "GlobalCallState.h"
#include "Ringer.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcGlobalCallState, "gonnect.callstate")

GlobalCallState::GlobalCallState(QObject *parent) : QObject{ parent }
{
    connect(this, &GlobalCallState::globalCallStateChanged, this, &GlobalCallState::updateRinger);
    connect(this, &GlobalCallState::callInForegroundChanged, this,
            &GlobalCallState::onCallInForegroundChanged);
}

void GlobalCallState::setGlobalCallState(const ICallState::States state)
{
    if (m_globalCallState != state) {
        m_globalCallState = state;
        emit globalCallStateChanged();

        // Debug output
        qCInfo(lcGlobalCallState).noquote().nospace()
                << "Global call state changed to: "
                << ICallState::statesAsStrings(m_globalCallState).join(", ");
    }
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
                emit globalCallStateObjectsChanged();
            }
        });
        connect(callStateObject, &ICallState::callStateChanged, this,
                &GlobalCallState::updateGlobalCallState);
        updateGlobalCallState();
        emit globalCallStateObjectsChanged();
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
        emit globalCallStateObjectsChanged();
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
}

void GlobalCallState::setIsPhoneConference(bool flag)
{
    if (flag != m_isPhoneConference) {
        m_isPhoneConference = flag;
        emit isPhoneConferenceChanged();
    }
}

void GlobalCallState::setRemoteContactInfo(const ContactInfo &info)
{
    m_remoteContactInfo = info;
    emit remoteContactInfoChanged();
}

void GlobalCallState::triggerHold()
{
    // Gather active call objects
    QSet<ICallState *> activeCalls;
    QSet<ICallState *> callsOnHold;
    QSet<ICallState *> callsNotOnHold;

    for (auto call : std::as_const(m_globalCallStateObjects)) {
        if (call->callState() & ICallState::State::CallActive) {
            activeCalls.insert(call);

            if (call->callState() & ICallState::State::OnHold) {
                callsOnHold.insert(call);
            } else {
                callsNotOnHold.insert(call);
            }
        }
    }

    // 0 calls - do nothing
    if (activeCalls.size() == 0) {
        return;
    }

    // 1 call - toggle
    if (activeCalls.size() == 1) {
        (*activeCalls.cbegin())->toggleHold();
        return;
    }

    // n calls, 1 active - hold active call, unhold next
    if (callsNotOnHold.size()) {
        (*activeCalls.cbegin())->toggleHold();
        (*callsOnHold.cbegin())->toggleHold();
        return;
    }

    // n calls, 0 active - unhold call in foreground
    if (m_callInForeground) {
        m_callInForeground->toggleHold();
        return;
    }

    // n calls, m active - hold all but the one in foreground
    for (auto call : std::as_const(callsNotOnHold)) {
        if (call != m_callInForeground) {
            call->toggleHold();
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
