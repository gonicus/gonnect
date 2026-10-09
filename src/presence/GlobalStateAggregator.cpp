#include "GlobalStateAggregator.h"

GlobalStateAggregator::GlobalStateAggregator(QObject *parent) : QObject{ parent } { }

void GlobalStateAggregator::setPresenceState(PresenceState::State state)
{
    if (m_presenceState != state) {
        m_presenceState = state;
        Q_EMIT presenceStateChanged();
    }
}
