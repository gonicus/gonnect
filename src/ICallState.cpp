#include "ICallState.h"
#include "GlobalCallState.h"

#ifdef Q_OS_LINUX
#  include "StateManager.h"
#  include "GOnnectDBusAPI.h"
#endif

#include <qmetaobject.h>
#include <QUuid>

QString ICallState::callStateToString(const State &state)
{
    return QMetaEnum::fromType<States>().valueToKey(static_cast<int>(state));
}

QList<QByteArray> ICallState::statesAsStrings(const ICallState::States &states)
{
    return QMetaEnum::fromType<State>().valueToKeys(states).split('|');
}

ICallState::ICallState(QObject *parent) : QObject{ parent }
{
    GlobalCallState::instance().registerCallStateObject(this);
    m_id = QUuid::createUuid().toString(QUuid::WithoutBraces);

#ifdef Q_OS_LINUX
    StateManager::instance().apiEndpoint()->registerCallState(this);
#endif
}

void ICallState::toggleHold()
{
    toggleHoldImpl();

    if (m_callState & State::OnHold) {
        removeCallState(State::OnHold);
    } else {
        addCallState(State::OnHold);
    }
}

void ICallState::setCallState(const ICallState::States &callState)
{
    if (m_callState != callState) {
        const auto oldState = m_callState;
        m_callState = callState;
        Q_EMIT callStateChanged(callState, oldState);
    }
}

void ICallState::addCallState(const ICallState::States &states)
{
    setCallState(m_callState | states);
}

void ICallState::removeCallState(const ICallState::States &states)
{
    setCallState(m_callState & ~states);
}
