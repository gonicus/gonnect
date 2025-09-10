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

    void set(const QString &key, const QString &secret, CredentialsResponse callback);
    void get(const QString &key, CredentialsResponse callback);

    ~Credentials() = default;

Q_SIGNALS:
    void initializedChanged();

private:
    explicit Credentials(QObject *parent = nullptr);

    QList<QKeychain::ReadPasswordJob *> m_readCredentialJobs;
    QList<QKeychain::WritePasswordJob *> m_writeCredentialJobs;

    bool m_initialized = false;
};
