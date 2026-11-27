#include "ConferenceUser.h"

#include <QMetaEnum>

QString ConferenceUser::userRoleToString(const Role role)
{
    return QMetaEnum::fromType<ConferenceUser::Role>().valueToKey(static_cast<int>(role));
}

ConferenceUser::ConferenceUser(const QString &id, const QString &displayName,
                               ConferenceUser::Role role, QObject *parent)
    : QObject{ parent }, m_id{ id }, m_displayName{ displayName }, m_role{ role }
{
}

void ConferenceUser::setRole(Role newRole)
{
    if (m_role != newRole) {
        m_role = newRole;
        Q_EMIT roleChanged();
    }
}

void ConferenceUser::setDisplayName(const QString &displayName)
{
    if (m_displayName != displayName) {
        m_displayName = displayName;
        Q_EMIT displayNameChanged();
    }
}

void ConferenceUser::setAudioMuted(bool isMuted)
{
    if (m_isAudioMuted != isMuted) {
        m_isAudioMuted = isMuted;
        Q_EMIT isAudioMutedChanged();
    }
}

void ConferenceUser::setVideoMuted(bool isMuted)
{
    if (m_isVideoMuted != isMuted) {
        m_isVideoMuted = isMuted;
        Q_EMIT isVideoMutedChanged();
    }
}

void ConferenceUser::setHandRaisedDateTime(const QDateTime &dateTime)
{
    if (m_handRaisedDateTime != dateTime) {
        m_handRaisedDateTime = dateTime;
        Q_EMIT handRaisedDateTimeChanged();
    }
}

void ConferenceUser::setDominantSpeaker(bool isDominantSpeaker)
{
    if (m_isDominantSpeaker != isDominantSpeaker) {
        m_isDominantSpeaker = isDominantSpeaker;
        Q_EMIT isDominantSpeakerChanged();
    }
}
