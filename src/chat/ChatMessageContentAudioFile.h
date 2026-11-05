#pragma once

#include <QObject>
#include <QQmlEngine>
#include "ChatMessageContentFile.h"

class ChatMessageContentAudioFile : public ChatMessageContentFile
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    explicit ChatMessageContentAudioFile(const QString &audioFilePath,
                                         const QString &audioFileName = "",
                                         QObject *parent = nullptr)
        : ChatMessageContentFile{ audioFilePath, audioFileName, parent }
    {
    }
};
