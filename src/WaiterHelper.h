#pragma once

#include "SecretResponse.h"

#include <QObject>
#include <qt6keychain/keychain.h>

class SecretWaiter : public QObject
{
    Q_OBJECT
public:
    SecretWaiter(QObject *parent = nullptr) : QObject{ parent } { }
Q_SIGNALS:
    void received(SecretResponse response);
};

class CredentialsWaiter : public QObject
{
    Q_OBJECT
public:
    CredentialsWaiter(QObject *parent = nullptr) : QObject{ parent } { }
Q_SIGNALS:
    void received(QKeychain::Error error, const QString &secret);
};

class PasswordWaiter : public QObject
{
    Q_OBJECT
public:
    PasswordWaiter(QObject *parent = nullptr) : QObject{ parent } { }
Q_SIGNALS:
    void received(const QString &password);
};
