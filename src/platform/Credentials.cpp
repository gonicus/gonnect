#include <fcntl.h>
#include <stdlib.h>
#include <QLoggingCategory>
#include "KeychainSettings.h"
#include "Credentials.h"

#ifdef Q_OS_FLATPAK
#  include "SecretPortal.h"
#endif

Q_LOGGING_CATEGORY(lcCredentials, "gonnect.credentials")

Credentials::Credentials(QObject *parent) : QObject(parent)
{
#ifdef Q_OS_FLATPAK
    SecretPortal::instance().initialize();
#endif
}

void Credentials::setIsInitialized(bool value)
{
    if (m_initialized != value) {
        m_initialized = value;
        Q_EMIT initializedChanged();
    }
}

void Credentials::initialize()
{
#ifdef Q_OS_FLATPAK
    auto &sp = SecretPortal::instance();
    if (sp.isValid()) {
        if (sp.isInitialized()) {
            m_isSecretPortalInitialized = true;
            setIsInitialized(true);
            return;

        } else if (!sp.hasTriedInitialization()) {
            connect(
                    &sp, &SecretPortal::initializedChanged, this,
                    [this]() {
                        m_isSecretPortalInitialized = SecretPortal::instance().isInitialized();

                        // Regular QKeychain is available anyway
                        setIsInitialized(true);
                    },
                    Qt::ConnectionType::SingleShotConnection);
            return;
        }
    }
#endif

    setIsInitialized(true);
}

void Credentials::set(const QString &key, const QString &secret, CredentialsResponse callback)
{
    auto writeJob = new QKeychain::WritePasswordJob(APP_ID);
    writeJob->setAutoDelete(false);
    writeJob->setKey(key);
    m_writeCredentialJobs.push_back(writeJob);

    connect(
            writeJob, &QKeychain::WritePasswordJob::finished, this,
            [this, writeJob, callback]() {
                if (writeJob->error()) {
                    callback(true,
                             tr("storing credentials failed: %1")
                                     .arg(qPrintable(writeJob->errorString())));
                } else {
                    callback(false, "");
                }

                m_writeCredentialJobs.removeAll(writeJob);
                writeJob->deleteLater();
            },
            Qt::QueuedConnection);

    writeJob->setTextData(secret);
    writeJob->start();
}

void Credentials::get(const QString &key, CredentialsResponse callback)
{
    auto readJob = new QKeychain::ReadPasswordJob(APP_ID);
    readJob->setAutoDelete(false);
    readJob->setKey(key);
    m_readCredentialJobs.push_back(readJob);

    QObject::connect(
            readJob, &QKeychain::ReadPasswordJob::finished, this,
            [this, readJob, key, callback]() {
                auto error = readJob->error();
                QString secret = readJob->textData();

                // Key is not present in keychain? Try to update it from Flatpak portal
                if (m_isSecretPortalInitialized
                    && (error == QKeychain::EntryNotFound || secret.isEmpty())) {
                    KeychainSettings keychainSettings;

#ifdef Q_OS_FLATPAK
                    auto &sp = SecretPortal::instance();
                    if (sp.isValid()) {
                        const auto encryptedSecret = keychainSettings.value(key, "").toString();
                        secret = sp.decrypt(encryptedSecret);

                        if (!secret.isEmpty()) {

                            set(key, secret, [key](bool error, const QString &misc) {
                                if (error) {
                                    qCCritical(lcCredentials)
                                            << "failed to update keychain credentials for" << key
                                            << "-" << misc;
                                }
                            });
                        }
                    }
#endif

                    callback(false, secret);
                } else if (error == QKeychain::EntryNotFound) {
                    callback(false, "inval!d");
                } else if (error != QKeychain::NoError) {
                    callback(true,
                             tr("reading credentials failed: %1")
                                     .arg(qPrintable(readJob->errorString())));
                } else {
                    callback(false, secret);
                }

                m_readCredentialJobs.removeAll(readJob);
                readJob->deleteLater();
            },
            Qt::QueuedConnection);

    readJob->start();
}
