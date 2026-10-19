#pragma once

#include <QString>
#include <QDateTime>

struct TranscriptionChunk
{
    QString messageId;
    QString participantId;
    QString participantName;
    QString participantAvatarUrl;
    QString language;
    QString finalText;
    QString stableText;
    QString unstableText;
    QDateTime receivedAt;
};
