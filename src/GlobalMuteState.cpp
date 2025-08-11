#include "GlobalMuteState.h"
#include "GlobalCallState.h"

GlobalMuteState::GlobalMuteState(QObject *parent) : QObject{ parent } { }

void GlobalMuteState::toggleMute(const QString &tag)
{
    m_isMuted = !m_isMuted;
    emit isMutedChangedWithTag(m_isMuted, tag);
    emit isMutedChanged();
}

void GlobalMuteState::reset()
{
    m_isMuted = false;
    emit isMutedChangedWithTag(m_isMuted, "");
    emit isMutedChanged();
}
