#include "CertHelper.h"
#include "ReadOnlyConfdSettings.h"

#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QSslKey>
#include <QQuickWebEngineProfile>
#include <QWebEngineClientCertificateStore>

Q_LOGGING_CATEGORY(lcCertHelper, "gonnect.helper.CertHelper")

CertHelper::CertHelper(QObject *parent) : QObject{ parent } { }

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

    ReadOnlyConfdSettings settings;
    const auto pathList = settings.value("generic/clientCertificates").toStringList();

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
        addClientCert(profile, fileInfo.absoluteFilePath());
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

void CertHelper::addClientCert(QQuickWebEngineProfile *profile, const QString &path) const
{
    Q_CHECK_PTR(profile);

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qCCritical(lcCertHelper) << "Unable to open" << path << ":" << file.errorString();
        return;
    }

    qCInfo(lcCertHelper) << "Reading client certificate(s) from" << path;

    const QByteArray data = file.readAll();

    switch (detectFormat(data)) {

    case Format::Pem:
        addClientCertPem(profile, data);
        break;

    case Format::Pkcs12:
        addClientCertP12(profile, data);
        break;

    case Format::Unknown:
    default:
        qCCritical(lcCertHelper) << "Unable to determine format of client certificate file" << path;
        return;
    }
}

void CertHelper::addClientCertPem(QQuickWebEngineProfile *profile, const QByteArray &data) const
{
    Q_CHECK_PTR(profile);

    const auto certs = QSslCertificate::fromData(data, QSsl::Pem);
    if (certs.isEmpty()) {
        qCCritical(lcCertHelper) << "Unable to find a certificate in PEM file";
        return;
    }

    QSslKey key(data, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
    if (key.isNull()) {
        key = QSslKey(data, QSsl::Ec, QSsl::Pem, QSsl::PrivateKey);
    }
    if (key.isNull()) {
        qCCritical(lcCertHelper) << "No readable private key in PEM file";
        return;
    }

    addToStore(profile, certs.first(), key);
}

void CertHelper::addClientCertP12(QQuickWebEngineProfile *profile, const QByteArray &data) const
{
    Q_CHECK_PTR(profile);

    QByteArray copy(data);
    QBuffer buffer(&copy);
    buffer.open(QIODevice::ReadOnly);

    QSslKey key;
    QSslCertificate cert;
    QList<QSslCertificate> chain;

    Q_UNUSED(chain) // intermediate CA not supported atm

    if (!QSslCertificate::importPkcs12(&buffer, &key, &cert, &chain)) {
        qCCritical(lcCertHelper) << "PKCS#12 import failed (corrupted or DER certificate)";
        return;
    }

    addToStore(profile, cert, key);
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
