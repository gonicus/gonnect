#pragma once

#include <QStringList>
#include <QLocale>

class PhoneCodeLookup
{

public:
    explicit PhoneCodeLookup() = delete;

    static QStringList countryNameFromPhoneNumber(const QString &phoneNumber);

    static QString cityNameFromPhoneNumber(const QString &phoneNumber);
};
