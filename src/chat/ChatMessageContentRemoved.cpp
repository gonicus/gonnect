#include "ChatMessageContentRemoved.h"

ChatMessageContentRemoved::ChatMessageContentRemoved(QObject *parent) : QObject{ parent } { }

void ChatMessageContentRemoved::setReason(const QString &reason)
{
    if (m_reason != reason) {
        m_reason = reason;
        Q_EMIT reasonChanged();
    }
}
