#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QSslCertificate>

class QQuickWebEngineProfile;

class CertHelper : public QObject
{
    Q_OBJECT

public:
    static CertHelper &instance()
    {
        static CertHelper helper;
        return helper;
    }

    /// List of file paths to configured CA files. Each is checked to exist and be readable. Might
    /// be empty.
    Q_INVOKABLE QStringList caCertPaths();

    /// Get a list of the configured CA certificates; empty when none are configured or could be
    /// loaded.
    const QList<QSslCertificate> &caCerts();

    /// Add all custom client certificates from settings to the given profile.
    Q_INVOKABLE void addClientCerts(QQuickWebEngineProfile *profile) const;

private:
    enum class Format { Unknown, Pem, Pkcs12 };

    explicit CertHelper(QObject *parent = nullptr);

    void ensureCaInitialized();
    bool isReadableFile(const QString &path) const;
    CertHelper::Format detectFormat(const QByteArray &data) const;

    void addClientCert(QQuickWebEngineProfile *profile, const QString &path) const;
    void addClientCertPerm(QQuickWebEngineProfile *profile, const QByteArray &data) const;
    void addClientCertP12(QQuickWebEngineProfile *profile, const QByteArray &data) const;
    void addToStore(QQuickWebEngineProfile *profile, const QSslCertificate &cert,
                    const QSslKey &key) const;

    QStringList m_caPaths;
    QList<QSslCertificate> m_caCerts;

    bool m_isCaInitialized = false;
};

class CertHelperWrapper
{
    Q_GADGET
    QML_FOREIGN(CertHelper)
    QML_NAMED_ELEMENT(CertHelper)
    QML_SINGLETON

public:
    static CertHelper *create(QQmlEngine *, QJSEngine *)
    {
        QQmlEngine::setObjectOwnership(&CertHelper::instance(), QQmlEngine::CppOwnership);
        return &CertHelper::instance();
    }

private:
    CertHelperWrapper() = default;
};
