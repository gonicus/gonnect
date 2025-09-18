#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <QLoggingCategory>
#include "KeychainSettings.h"
#include "Credentials.h"

#ifdef Q_OS_LINUX
#  include "SecretPortal.h"
#endif

Q_LOGGING_CATEGORY(lcCredentials, "gonnect.credentials")

Credentials::Credentials(QObject *parent) : QObject(parent)
{
#ifdef Q_OS_LINUX
    SecretPortal::instance().initialize();
#endif
}

void Credentials::initialize()
{
#ifdef Q_OS_LINUX
    auto &sp = SecretPortal::instance();
    if (!sp.isValid()) {
        qCFatal(lcCredentials) << "flatpak Secret Portal is not valid - bailing out";
    }

    if (sp.isInitialized()) {
        m_initialized = true;
        Q_EMIT initializedChanged();
    } else {
        connect(
                &sp, &SecretPortal::initializedChanged, this,
                [this]() {
                    bool isInitialized = SecretPortal::instance().isInitialized();
                    if (isInitialized != m_initialized) {
                        m_initialized = isInitialized;
                        Q_EMIT initializedChanged();
                    }
                },
                Qt::ConnectionType::SingleShotConnection);
    }
#else
    m_initialized = true;
    Q_EMIT initializedChanged();
#endif
}

void Credentials::set(const QString &key, const QString &secret, CredentialsResponse callback)
{
    auto writeJob = new QKeychain::WritePasswordJob(FLATPAK_APP_ID);
    writeJob->setAutoDelete(false);
    writeJob->setKey(key);
    m_writeCredentialJobs.push_back(writeJob);

    connect(writeJob, &QKeychain::WritePasswordJob::finished, this, [this, writeJob, callback]() {
        if (writeJob->error()) {
            callback(true,
                     tr("storing credentials failed: %1").arg(qPrintable(writeJob->errorString())));
        } else {
            callback(false, "");
        }

        m_writeCredentialJobs.removeAll(writeJob);
        delete writeJob;
    });

    writeJob->setTextData(secret);
    writeJob->start();
}

void Credentials::get(const QString &key, CredentialsResponse callback)
{
    auto readJob = new QKeychain::ReadPasswordJob(FLATPAK_APP_ID);
    readJob->setAutoDelete(false);
    readJob->setKey(key);
    m_readCredentialJobs.push_back(readJob);

    QObject::connect(
            readJob, &QKeychain::ReadPasswordJob::finished, this, [this, readJob, key, callback]() {
                auto error = readJob->error();
                QString secret = readJob->textData();

                // Key is not present in keychain? Try to update it from Flatpak portal
                if (error == QKeychain::EntryNotFound || secret.isEmpty()) {
                    KeychainSettings keychainSettings;

#ifdef Q_OS_LINUX
                    const auto encryptedSecret = keychainSettings.value(key, "").toString();
                    secret = SecretPortal::instance().decrypt(encryptedSecret);

                    if (!secret.isEmpty()) {

                        set(key, secret, [key](bool error, const QString &misc) {
                            if (error) {
                                qCCritical(lcCredentials)
                                        << "failed to update keychain credentials for" << key << "-"
                                        << misc;
                            }
                        });
                    }
#endif

                    callback(false, secret);
                } else if (error != QKeychain::NoError) {
                    callback(true,
                             tr("reading credentials failed: %1")
                                     .arg(qPrintable(readJob->errorString())));
                } else {
                    callback(false, secret);
                }

                m_readCredentialJobs.removeAll(readJob);
                delete readJob;
            });

    readJob->start();
}
