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
        qCDebug(lcCredentials) << "Flatpak secret portal found.";
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
    } else {
        qCDebug(lcCredentials) << "Flatpak secret portal not found.";
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
                    callback(ResponseState_Error,
                             tr("storing credentials failed: %1")
                                     .arg(qPrintable(writeJob->errorString())));
                } else {
                    callback(ResponseState_Valid, "");
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

                qCDebug(lcCredentials) << "handling QKeychain respond - key:" << key
                                       << "error:" << error << "secretEmpty:" << secret.isEmpty();

                // Key is not present in keychain? Try to update it from Flatpak portal
                if (m_isSecretPortalInitialized && (error == QKeychain::EntryNotFound)) {
                    KeychainSettings keychainSettings;

                    qCDebug(lcCredentials) << "no secret from QKeychain; try secret portal";

#ifdef Q_OS_FLATPAK
                    qCDebug(lcCredentials) << "checking if secret portal is valid";

                    auto &sp = SecretPortal::instance();
                    if (sp.isValid()) {
                        qCDebug(lcCredentials) << "secret portal is valid";

                        // Create a default value with a low possibility of collision with an actual
                        // secret.
                        const QString VALUE_DEFAULT =
                                "ebeet5iayaefoxee4ies8vueChaegaeH1zai0ANgohSe3iethu";

                        const auto encryptedSecret =
                                keychainSettings.value(key, VALUE_DEFAULT).toString();

                        if (encryptedSecret != VALUE_DEFAULT) {
                            // Got a secret from the portal, so migrate it to QKeychain.
                            secret = sp.decrypt(encryptedSecret);

                            qCDebug(lcCredentials) << "migrate secret to QKeychain";

                            set(key, secret, [key](bool error, const QString &misc) {
                                if (error) {
                                    qCCritical(lcCredentials)
                                            << "failed to update keychain credentials for" << key
                                            << "-" << misc;
                                }
                            });

                            callback(ResponseState_Valid, secret);
                        }
                    }
#endif

                    callback(ResponseState_Empty, secret);
                } else if (error == QKeychain::EntryNotFound) {
                    qCDebug(lcCredentials) << "no secret found by QKeychain";

                    callback(ResponseState_Empty, "");
                } else if (error != QKeychain::NoError) {
                    callback(ResponseState_Error,
                             tr("reading credentials failed: %1")
                                     .arg(qPrintable(readJob->errorString())));
                } else {
                    callback(ResponseState_Valid, secret);
                }

                m_readCredentialJobs.removeAll(readJob);
                readJob->deleteLater();
            },
            Qt::QueuedConnection);

    readJob->start();
}
