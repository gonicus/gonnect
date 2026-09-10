#include "ChatMessageContentUserStateChange.h"

ChatMessageContentUserStateChange::ChatMessageContentUserStateChange(State state,
                                                                     const QString &affectedUserId,
                                                                     QObject *parent)
    : QObject{ parent }, m_state{ state }, m_affectedUserId{ affectedUserId }
{
}

void ChatMessageContentUserStateChange::setSetState(const State state)
{
    if (m_state != state) {
        m_state = state;
        Q_EMIT stateChanged();
    }
}

void ChatMessageContentUserStateChange::setAffectedUserId(const QString &id)
{
    if (m_affectedUserId != id) {
        m_affectedUserId = id;
        Q_EMIT affectedUserIdChanged();
    }
}
