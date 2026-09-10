#pragma once

#include <QObject>
#include <QtQml>

class PresenceState : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    explicit PresenceState(QObject *parent = nullptr) : QObject{ parent } { };

    enum class State { Unknown, Offline, Away, Busy, Available, Ringing };
    Q_ENUM(State)
};
