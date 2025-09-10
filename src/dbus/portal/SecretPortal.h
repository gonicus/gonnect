#pragma once

#include "AbstractPortal.h"

#define SECRET_PORTAL_INTERFACE "org.freedesktop.portal.Secret"
#define SECRET_MAX_LEN 128

class SecretPortal : public AbstractPortal
{
    Q_OBJECT
    Q_DISABLE_COPY(SecretPortal)

public:
    Q_REQUIRED_RESULT static SecretPortal &instance()
    {
        static SecretPortal *_instance = nullptr;

        if (_instance == nullptr) {
            _instance = new SecretPortal();
        }

        return *_instance;
    }

    void initialize();

    bool isInitialized() { return m_initialized; }
    bool hasInstanceSecret() { return !m_instanceSecret.isEmpty(); }

    QString encrypt(const QString &plainText);
    QString decrypt(const QString &cipherText);

    ~SecretPortal() = default;

Q_SIGNALS:
    void initializedChanged();

private:
    explicit SecretPortal(QObject *parent = nullptr);
    void RetrieveSecret(PortalResponse callback);

    QByteArray m_instanceSecret;
    QByteArray m_iva;

    int m_fds[2];

    bool m_initialized = false;
    bool m_supported = false;
};
