#pragma once

#include <QObject>
#include <QQueue>
#include <qt6keychain/keychain.h>

typedef std::function<void(QKeychain::Error error, const QString &secret, const QString &message)>
        CredentialsResponse;

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

    void setIsInitialized(bool value);

    void enqueueJob(const QString &key, std::function<void()> fn);
    void runNextJob();

    QList<QKeychain::ReadPasswordJob *> m_readCredentialJobs;
    QList<QKeychain::WritePasswordJob *> m_writeCredentialJobs;

    QQueue<std::function<void()>> m_jobQueue;
    bool m_jobRunning = false;

    bool m_initialized = false;
    bool m_isSecretPortalInitialized = false;
};
