#pragma once

#include "MatrixEvent.h"
#include "ChatMessage.h"

class MatrixMessageEvent : public MatrixEvent, public ChatMessage
{
    Q_OBJECT
public:
    explicit MatrixMessageEvent(const QString &eventId, const QString &roomId,
                                const QString &senderId, const QDateTime &dateTime,
                                const QString &message, ChatMessage::Flags flags,
                                QObject *parent = nullptr);
};
