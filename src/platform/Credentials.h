#pragma once

#include <QObject>
#include <qt6keychain/keychain.h>

typedef std::function<void(bool error, const QString &misc)> CredentialsResponse;

class Credentials : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Credentials)

public:
    Q_REQUIRED_RESULT static Credentials &instance()
    {
        static Credentials *_instance = nullptr;

        if (_instance == nullptr) {
            _instance = new Credentials();
        }

        return *_instance;
    }

    void initialize();

    bool isInitialized() { return m_initialized; }

    void set(const QString &key, const QString& secret, CredentialsResponse callback);
    void get(const QString &key, CredentialsResponse callback);

    ~Credentials() = default;

signals:
    void initializedChanged();

private:
    explicit Credentials(QObject *parent = nullptr);

    QKeychain::ReadPasswordJob m_readCredentialJob;
    QKeychain::WritePasswordJob m_writeCredentialJob;
    QKeychain::DeletePasswordJob m_deleteCredentialJob;

    bool m_initialized = false;
};
