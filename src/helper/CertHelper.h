#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QSslCertificate>

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

private:
    explicit CertHelper(QObject *parent = nullptr);
    void ensureCaInitialized();

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
