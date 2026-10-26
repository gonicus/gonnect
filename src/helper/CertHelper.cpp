#include "CertHelper.h"
#include "ReadOnlyConfdSettings.h"
#include "Credentials.h"
#include "ViewHelper.h"

#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QSslKey>
#include <QQuickWebEngineProfile>
#include <QWebEngineClientCertificateStore>

Q_LOGGING_CATEGORY(lcCertHelper, "gonnect.helper.CertHelper")

CertHelper::CertHelper(QObject *parent) : QObject{ parent }
{

    connect(&ViewHelper::instance(), &ViewHelper::passwordResponded, this,
            &CertHelper::onPasswordResponse);
}

void CertHelper::initializeClientCerts()
{
    if (m_areClientCertsInitialized) {
        qCWarning(lcCertHelper) << "Client certificates have already been loaded";
        return;
    }

    m_isCreatingClientCertificateEntries = true;

    ReadOnlyConfdSettings settings;
    const auto pathList = settings.value("generic/clientCertificates").toStringList();

    m_clientCerts.reserve(pathList.size());

    for (const auto &path : pathList) {
        if (path.isEmpty()) {
            qCWarning(lcCertHelper) << "Ignoring empty client certificate path";
            continue;
        }

        if (!isReadableFile(path)) {
            qCCritical(lcCertHelper) << "Unable to read client cerificate file" << path;
            continue;
        }

        const QFileInfo fileInfo(path);
        loadCert(fileInfo.absoluteFilePath());
    }

    m_isCreatingClientCertificateEntries = false;

    checkClientCertsInitialized();
}

QStringList CertHelper::caCertPaths()
{
    ensureCaInitialized();
    return m_caPaths;
}

const QList<QSslCertificate> &CertHelper::caCerts()
{
    ensureCaInitialized();
    return m_caCerts;
}

void CertHelper::addClientCerts(QQuickWebEngineProfile *profile) const
{
    if (!profile) {
        qCWarning(lcCertHelper) << "Cannot add certificates to null profile";
        return;
    }
    if (!m_areClientCertsInitialized) {
        qCWarning(lcCertHelper) << "Client certificates have not been initialized yet - try again "
                                   "after clientCertsInitialized becoming true";
        return;
    }

    for (const auto &entry : std::as_const(m_clientCerts)) {
        addToStore(profile, entry.cert, entry.key);
    }
}

void CertHelper::onPasswordResponse(const QString id, const QString password)
{
    if (m_pendingPassphraseRequests.contains(id)) {
        const auto entry = m_pendingPassphraseRequests.take(id);
        (this->*entry.callback)(entry.index, password);
    }
}

void CertHelper::ensureCaInitialized()
{
    if (m_isCaInitialized) {
        return;
    }
    m_isCaInitialized = true;

    ReadOnlyConfdSettings settings;
    const auto pathList = settings.value("generic/caFiles").toStringList();

    m_caPaths.reserve(pathList.size());
    m_caCerts.reserve(pathList.size());

    for (const auto &path : pathList) {
        if (!isReadableFile(path)) {
            qCCritical(lcCertHelper) << "Unable to read CA file" << path;
            continue;
        }

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qCCritical(lcCertHelper) << "Unable to read CA file" << path;
            continue;
        }

        m_caPaths.append(QFileInfo(file).absoluteFilePath());

        QSslCertificate cert(file.readAll());
        file.close();

        m_caCerts.append(cert);

        qCInfo(lcCertHelper) << "Loaded CA certificate" << m_caPaths.last();
    }

    qCInfo(lcCertHelper) << "CA certificate initialization completed, loaded" << m_caPaths.size()
                         << "certificates";
}

bool CertHelper::isReadableFile(const QString &path) const
{
    if (path.isEmpty()) {
        return false;
    }

    const QFileInfo fileInfo(path);
    return fileInfo.isFile() && fileInfo.isReadable();
}

CertHelper::Format CertHelper::detectFormat(const QByteArray &data) const
{
    if (data.contains("-----BEGIN ")) {
        return Format::Pem;
    }

    if (!data.isEmpty() && static_cast<uchar>(data.at(0)) == 0x30) {
        return Format::Pkcs12;
    }

    return Format::Unknown;
}

void CertHelper::loadCert(const QString &path)
{

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qCCritical(lcCertHelper) << "Unable to open" << path << ":" << file.errorString();
        return;
    }

    qCInfo(lcCertHelper) << "Reading client certificate(s) from" << path;

    m_clientCerts.append(CertEntry());
    const qsizetype index = m_clientCerts.size() - 1;
    auto &entry = m_clientCerts[index];
    entry.filePath = path;
    entry.data = file.readAll();
    entry.format = detectFormat(entry.data);

    switch (entry.format) {

    case Format::Pem:
        loadClientCertPem(index);
        break;

    case Format::Pkcs12:
        loadClientCertP12(index);
        break;

    case Format::Unknown:
    default:
        m_clientCerts.removeAt(index);
        qCCritical(lcCertHelper) << "Unable to determine format of client certificate file" << path;
        return;
    }
}

void CertHelper::loadClientCertPem(const qsizetype index, const QString &passphrase)
{
    auto &entry = m_clientCerts[index];
    const auto certs = QSslCertificate::fromData(entry.data, QSsl::Pem);
    if (certs.isEmpty()) {
        qCCritical(lcCertHelper) << "Unable to find a certificate in PEM file";
        return;
    }
    entry.cert = certs.first();

    if (passphrase.isEmpty()
        && (entry.requiredPassphrase || doesPemRequirePassphrase(entry.data))) {
        entry.requiredPassphrase = true;
        requestPasswordForClientCert(index, &CertHelper::loadClientCertPem);
        return;
    }

    QSslKey key(entry.data, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey, passphrase.toUtf8());
    if (key.isNull()) {
        key = QSslKey(entry.data, QSsl::Ec, QSsl::Pem, QSsl::PrivateKey, passphrase.toUtf8());
    }
    if (key.isNull()) {
        if (entry.requiredPassphrase && !passphrase.isEmpty()) {
            qCWarning(lcCertHelper)
                    << "Wrong passphrase for PEM key (likely) or corrupted data (unlikely)";
            requestPasswordForClientCert(index, &CertHelper::loadClientCertPem, true);
        } else {
            qCCritical(lcCertHelper) << "No readable private key in PEM file - ignoring";
            entry.requiredPassphrase = false;
            checkClientCertsInitialized();
        }
        return;
    }

    if (!passphrase.isEmpty()) {
        savePassphrase(entry.filePath, passphrase);
    }

    entry.key = key;
    entry.decrypted = true;
    checkClientCertsInitialized();
}

bool CertHelper::doesPemRequirePassphrase(const QByteArray &data) const
{
    return data.contains("ENCRYPTED PRIVATE KEY") || data.contains("Proc-Type: 4,ENCRYPTED");
}

void CertHelper::loadClientCertP12(const qsizetype index, const QString &passphrase)
{
    auto &entry = m_clientCerts[index];

    QByteArray copy(entry.data);
    QBuffer buffer(&copy);
    buffer.open(QIODevice::ReadOnly);

    QList<QSslCertificate> chain;

    Q_UNUSED(chain) // intermediate CA not supported atm

    if (!QSslCertificate::importPkcs12(&buffer, &entry.key, &entry.cert, &chain,
                                       passphrase.toUtf8())) {
        entry.requiredPassphrase = true;
        requestPasswordForClientCert(index, &CertHelper::loadClientCertP12, !passphrase.isEmpty());
        return;
    }

    if (!passphrase.isEmpty()) {
        savePassphrase(entry.filePath, passphrase);
    }

    entry.decrypted = true;
    checkClientCertsInitialized();
}

void CertHelper::requestPasswordForClientCert(const qsizetype index, PassphraseCallback callback,
                                              bool enforceNewEntry)
{
    const QString settingsKey = secretKeyForClientCertPath(m_clientCerts.at(index).filePath);

    const auto request = [this, index, callback, settingsKey]() {
        const auto reqId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        m_pendingPassphraseRequests.insert(reqId, { index, callback, settingsKey });
        ViewHelper::instance().requestPassword(reqId, m_clientCerts.at(index).filePath);
    };

    if (enforceNewEntry) {
        request();

    } else {
        Credentials::instance().get(
                settingsKey,
                [this, callback, request, settingsKey,
                 index](QKeychain::Error error, const QString &secret, const QString &) {
                    if (error == QKeychain::NoError && !secret.isEmpty()) {
                        (this->*callback)(index, secret);
                    } else {
                        request();
                    }
                });
    }
}

void CertHelper::addToStore(QQuickWebEngineProfile *profile, const QSslCertificate &cert,
                            const QSslKey &key) const
{
    Q_CHECK_PTR(profile);

    if (cert.expiryDate() < QDateTime::currentDateTimeUtc()) {
        qCCritical(lcCertHelper) << "Certificate expired";
        return;
    }

    profile->clientCertificateStore()->add(cert, key);
}

void CertHelper::checkClientCertsInitialized()
{
    if (m_areClientCertsInitialized || m_isCreatingClientCertificateEntries) {
        return;
    }

    for (const auto &entry : std::as_const(m_clientCerts)) {
        if (entry.requiredPassphrase && !entry.decrypted) {
            return;
        }
    }

    m_areClientCertsInitialized = true;
    Q_EMIT clientCertsInitializedChanged();
}

QString CertHelper::secretKeyForClientCertPath(const QString &path) const
{
    ReadOnlyConfdSettings settings;
    return QString("clientcert_%1/secret").arg(settings.hashForSettingsGroup(path));
}

void CertHelper::savePassphrase(const QString &path, const QString &secret) const
{
    const QString settingsKey = secretKeyForClientCertPath(path);
    Credentials::instance().set(
            settingsKey, secret,
            [settingsKey](QKeychain::Error error, const QString &, const QString &message) {
                if (error != QKeychain::NoError) {
                    qCCritical(lcCertHelper)
                            << "Failed to persist client certificate passphrase for" << settingsKey
                            << ":" << message;
                }
            });
}
