#pragma once

#include <QObject>
#include <QString>
#include <qqmlregistration.h>

struct MessageJumpInfo
{
    Q_GADGET
    QML_NAMED_ELEMENT(MessageJumpInfo)
    QML_VALUE_TYPE(MessageJumpInfo)
    QML_STRUCTURED_VALUE

    Q_PROPERTY(QString roomId MEMBER roomId FINAL)
    Q_PROPERTY(QString messageId MEMBER messageId FINAL)

public:
    bool operator==(const MessageJumpInfo &) const = default;

    Q_INVOKABLE bool isEmpty() const { return roomId.isEmpty() || messageId.isEmpty(); }

    QString roomId;
    QString messageId;
};
