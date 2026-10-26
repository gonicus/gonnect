#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QSslCertificate>
#include <QSslKey>

class QQuickWebEngineProfile;

class CertHelper : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool clientCertsInitialized READ clientCertsInitialized NOTIFY
                       clientCertsInitializedChanged FINAL)

public:
    static CertHelper &instance()
    {
        static CertHelper helper;
        return helper;
    }

    /// Load client certs from config. The property clientCertsInitialized is set to true
    /// afterwards. Calling it while already initalized does nothing.
    Q_INVOKABLE void initializeClientCerts();

    /// List of file paths to configured CA files. Each is checked to exist and be readable. Might
    /// be empty.
    Q_INVOKABLE QStringList caCertPaths();

    /// Get a list of the configured CA certificates; empty when none are configured or could be
    /// loaded.
    const QList<QSslCertificate> &caCerts();

    /// Add all custom client certificates from settings to the given profile. Does only work after
    /// clientCertsInitialized is true; aborts otherwise.
    Q_INVOKABLE void addClientCerts(QQuickWebEngineProfile *profile) const;

private Q_SLOTS:
    void onPasswordResponse(const QString id, const QString password);

private:
    using PassphraseCallback = void (CertHelper::*)(const qsizetype, const QString &);

    enum class Format { Unknown, Pem, Pkcs12 };

    struct CertEntry
    {
        QString filePath;
        QSslCertificate cert;
        QSslKey key;
        QByteArray data;
        Format format = Format::Unknown;
        bool requiredPassphrase = false;
        bool decrypted = false;
    };

    struct PassphraseRequestEntry
    {
        qsizetype index = -1;
        PassphraseCallback callback;
        QString settingsKey;
    };

    explicit CertHelper(QObject *parent = nullptr);

    bool clientCertsInitialized() const { return m_areClientCertsInitialized; }

    void ensureCaInitialized();
    bool isReadableFile(const QString &path) const;
    CertHelper::Format detectFormat(const QByteArray &data) const;

    void loadCert(const QString &path);
    void loadClientCertPem(const qsizetype index, const QString &passphrase = QString());
    void loadClientCertP12(const qsizetype index, const QString &passphrase = QString());

    void requestPasswordForClientCert(const qsizetype index, PassphraseCallback callback,
                                      bool enforceNewEntry = false);

    bool doesPemRequirePassphrase(const QByteArray &data) const;
    void checkClientCertsInitialized();
    QString secretKeyForClientCertPath(const QString &path) const;
    void savePassphrase(const QString &path, const QString &secret) const;

    void addToStore(QQuickWebEngineProfile *profile, const QSslCertificate &cert,
                    const QSslKey &key) const;

    QStringList m_caPaths;
    QList<QSslCertificate> m_caCerts;
    QList<CertEntry> m_clientCerts;
    QHash<QString, PassphraseRequestEntry> m_pendingPassphraseRequests;

    bool m_isCaInitialized = false;
    bool m_areClientCertsInitialized = false;
    bool m_isCreatingClientCertificateEntries = false;

Q_SIGNALS:
    void clientCertsInitializedChanged();
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
