#include "ConferenceParticipant.h"

#include <QMetaEnum>

QString ConferenceParticipant::participantRoleToString(const Role role)
{
    return QMetaEnum::fromType<ConferenceParticipant::Role>().valueToKey(static_cast<int>(role));
}

ConferenceParticipant::ConferenceParticipant(const QString &id, const QString &displayName,
                                             ConferenceParticipant::Role role, QObject *parent)
    : QObject{ parent }, m_id{ id }, m_displayName{ displayName }, m_role{ role }
{
}

void ConferenceParticipant::setRole(Role newRole)
{
    if (m_role != newRole) {
        m_role = newRole;
        Q_EMIT roleChanged();
    }
}
