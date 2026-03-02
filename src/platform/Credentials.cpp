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
        qCDebug(lcCredentials) << "initialized" << value;
        m_initialized = value;
        Q_EMIT initializedChanged();
    }
}

void Credentials::initialize()
{
    qCDebug(lcCredentials) << "initializing credential handler";

#ifdef Q_OS_FLATPAK
    auto &sp = SecretPortal::instance();
    if (sp.isValid()) {
        if (sp.isInitialized()) {
            qCDebug(lcCredentials) << "flatpak portal initialized and available";
            m_isSecretPortalInitialized = true;
            setIsInitialized(true);
            return;

        } else if (!sp.hasTriedInitialization()) {
            qCDebug(lcCredentials) << "flatpak portal available, still waiting for initialization";
            connect(
                    &sp, &SecretPortal::initializedChanged, this,
                    [this]() {
                        m_isSecretPortalInitialized = SecretPortal::instance().isInitialized();

                        qCDebug(lcCredentials)
                                << "flatpak portal"
                                << (m_isSecretPortalInitialized ? "is now initialized"
                                                                : "failed to initialze");

                        // The flatpak portal is only a fall back to extract existing credentials
                        // from earlier installations. Signalize that we're initialized here, to
                        // let the auth flows flow.
                        setIsInitialized(true);
                    },
                    Qt::ConnectionType::SingleShotConnection);
            return;
        } else {
            qCDebug(lcCredentials) << "flatpak portal not available, initialization failed";
        }
    } else {
        qCDebug(lcCredentials) << "flatpak portal not available";
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
            [this, key, writeJob, callback]() {
                auto error = writeJob->error();

                qCDebug(lcCredentials)
                        << "job writing credentials for" << key << "returned with" << error;

                if (error == QKeychain::NoError) {
                    callback(error, "", "");
                    qCDebug(lcCredentials) << "successfully stored credentials for" << key;
                } else {
                    callback(writeJob->error(), "",
                             tr("Storing credentials for %1 failed: %2")
                                     .arg(key)
                                     .arg(qPrintable(writeJob->errorString())));
                    qCDebug(lcCredentials) << "job writing credentials for" << key
                                           << "failed:" << writeJob->errorString();
                }

                m_writeCredentialJobs.removeAll(writeJob);
                writeJob->deleteLater();
            },
            Qt::QueuedConnection);

    qCDebug(lcCredentials) << "starting credential write job for" << key;
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
                QString message;
                QString secret = readJob->textData();
                qCDebug(lcCredentials)
                        << "job reading credentials for" << key << "returned with" << error
                        << "- secret is" << (secret.isEmpty() ? "empty" : "available");

                // Key is not present in keychain? Try to update it from Flatpak portal
                if (m_isSecretPortalInitialized && error == QKeychain::EntryNotFound) {
                    KeychainSettings keychainSettings;

#ifdef Q_OS_FLATPAK
                    auto &sp = SecretPortal::instance();
                    if (sp.isValid()) {
                        const auto encryptedSecret = keychainSettings.value(key, "").toString();
                        secret = sp.decrypt(encryptedSecret);

                        if (!secret.isEmpty()) {
                            qCDebug(lcCredentials)
                                    << "we have a secret portal, checking if we've a secret there";

                            set(key, secret,
                                [callback, key](QKeychain::Error error, const QString &,
                                                const QString &message) {
                                    // Setting the secret is currently "out of the line". We don't
                                    // care if it fails, because it doesn't affect the flow.
                                    if (error != QKeychain::NoError) {
                                        qCCritical(lcCredentials)
                                                << "failed to update keychain credentials for"
                                                << key << "-" << message;
                                    }
                                });

                            error = QKeychain::NoError;
                        } else {
                            qCDebug(lcCredentials) << "secret portal has no password for us";
                        }
                    } else {
                        qCDebug(lcCredentials) << "trying to recover password from secret portal: "
                                                  "no valid portal available";
                    }
#endif
                } else if (error != QKeychain::NoError) {
                    message = tr("reading credentials for %1 failed: %2")
                                      .arg(key)
                                      .arg(qPrintable(readJob->errorString()));
                    qCDebug(lcCredentials) << "reading credentials for" << key
                                           << "failed:" << readJob->errorString();
                } else {
                    qCDebug(lcCredentials) << "reading credentials for" << key << "succeeded";
                }

                callback(error, secret, message);

                m_readCredentialJobs.removeAll(readJob);
                readJob->deleteLater();
            },
            Qt::QueuedConnection);

    qCDebug(lcCredentials) << "starting credential read job for" << key;
    readJob->start();
}
