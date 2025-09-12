#pragma once

#include "JsChatEvent.h"
#include "ChatMessage.h"

class JsChatMessageEvent : public JsChatEvent, public ChatMessage
{
    Q_OBJECT
public:
    explicit JsChatMessageEvent(const QString &eventId, const QString &roomId,
                                const QString &senderId, const QString &displayName,
                                const QDateTime &dateTime, const QString &message,
                                ChatMessage::Flags flags, QObject *parent = nullptr);
};
