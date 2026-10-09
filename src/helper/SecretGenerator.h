#pragma once

#include <QString>

class SecretGenerator
{
public:
    SecretGenerator() = delete;

    static QString generateSecret(quint8 length = 16);
};
