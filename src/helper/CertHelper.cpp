#include "CertHelper.h"
#include "ReadOnlyConfdSettings.h"

#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>

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
