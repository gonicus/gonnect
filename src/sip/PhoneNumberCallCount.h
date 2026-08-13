#pragma once

#include <QObject>

struct PhoneNumberCallCount : public QObject
{
    Q_OBJECT

public:
    explicit PhoneNumberCallCount(QObject *parent = nullptr) : QObject{ parent } { }

    QString phoneNumber;
    quint32 count = 0;
};
