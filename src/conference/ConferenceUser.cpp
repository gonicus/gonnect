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
