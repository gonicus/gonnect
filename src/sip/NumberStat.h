#pragma once
#include <QObject>
#include "NumberStats.h"

class Contact;

struct NumberStat : public QObject
{
    Q_OBJECT

public:
    explicit NumberStat(QObject *parent = nullptr) : QObject{ parent } { }

    QString phoneNumber;
    bool isFavorite = false;
    bool isBlocked = false;
    NumberStats::ContactType contactType = NumberStats::ContactType::PhoneNumber;
    Contact *contact = nullptr;
};
