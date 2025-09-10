#include "GlobalMuteState.h"
#include "GlobalCallState.h"

GlobalMuteState::GlobalMuteState(QObject *parent) : QObject{ parent } { }

void GlobalMuteState::toggleMute(const QString &tag)
{
    m_isMuted = !m_isMuted;
    Q_EMIT isMutedChangedWithTag(m_isMuted, tag);
    Q_EMIT isMutedChanged();
}

void GlobalMuteState::reset()
{
    m_isMuted = false;
    Q_EMIT isMutedChangedWithTag(m_isMuted, "");
    Q_EMIT isMutedChanged();
}
