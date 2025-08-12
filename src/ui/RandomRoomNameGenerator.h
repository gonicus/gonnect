#pragma once

#include <QObject>
#include <QQmlEngine>

class RandomRoomNameGenerator : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit RandomRoomNameGenerator(QObject *parent = nullptr);

    Q_INVOKABLE QString randomJitsiRoomName() const;

private:
    QString randomStringFrom(const QStringList &list) const;
};
