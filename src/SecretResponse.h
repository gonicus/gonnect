#pragma once

#include <QString>

struct SecretResponse
{
    SecretResponse(const QString &secret, bool hasError = false)
        : hasError{ hasError }, secret{ secret }
    {
    }
    bool hasError = false;
    QString secret;
};
