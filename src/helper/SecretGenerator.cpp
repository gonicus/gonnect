#include "SecretGenerator.h"

#include <QRandomGenerator>

QString SecretGenerator::generateSecret(quint8 length)
{
    static const QString allowed = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static const auto allowedSize = allowed.length();

    auto randGen = QRandomGenerator::securelySeeded();
    QString randomString;
    randomString.reserve(length);

    for (quint8 i = 0; i < length; i++) {
        randomString.append(allowed.at(randGen.bounded(0, allowedSize)));
    }

    return randomString;
}
