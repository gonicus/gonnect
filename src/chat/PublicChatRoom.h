#pragma once

#include <QString>
#include <qobjectdefs.h>
#include "IChatRoom.h"

/// Date holder object for chat rooms which are known and can be joined, but the client is not a
/// member yet.
struct PublicChatRoom
{
    Q_GADGET

public:
    QString roomId;
    QString displayName;
    QString topic;
    quint32 numberOfJoinedMembers = 0;
    IChatRoom::JoinRule joinRule = IChatRoom::JoinRule::Unknown;
};
