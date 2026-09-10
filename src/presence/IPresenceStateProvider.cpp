#include "IPresenceStateProvider.h"

IPresenceStateProvider::IPresenceStateProvider(QObject *parent) : QObject{ parent } { }

void IPresenceStateProvider::setPresenceState(PresenceState::State state)
{
    if (m_presenceState != state) {
        m_presenceState = state;
        Q_EMIT presenceStateChanged();
    }
}

void IPresenceStateProvider::setStateText(const QString &text)
{
    if (m_stateText != text) {
        m_stateText = text;
        Q_EMIT stateTextChanged();
    }
}
