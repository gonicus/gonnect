#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <QtCrypto>
#include "KeychainSettings.h"
#include "Credentials.h"

#ifdef Q_OS_LINUX
#  include "SecretPortal.h"
#endif

Q_LOGGING_CATEGORY(lcCredentials, "gonnect.credentials")

Credentials::Credentials(QObject *parent)
    : QObject(parent),
      m_readCredentialJob(FLATPAK_APP_ID),
      m_writeCredentialJob(FLATPAK_APP_ID),
      m_deleteCredentialJob(FLATPAK_APP_ID)
{
#ifdef Q_OS_LINUX
    SecretPortal::instance().initialize();
#endif

    m_readCredentialJob.setAutoDelete(false);
    m_writeCredentialJob.setAutoDelete(false);
    m_deleteCredentialJob.setAutoDelete(false);
}

void Credentials::initialize()
{
#ifdef Q_OS_LINUX
    auto &sp = SecretPortal::instance();
    if (sp.isValid()) {
        if (sp.isInitialized()) {
            m_initialized = true;
            emit initializedChanged();
        } else {
            connect(
                    &sp, &SecretPortal::initializedChanged, this,
                    [this]() {
                        bool isInitialized = SecretPortal::instance().isInitialized();
                        if (isInitialized != m_initialized) {
                            m_initialized = isInitialized;
                            emit initializedChanged();
                        }
                    },
                    Qt::ConnectionType::SingleShotConnection);
        }
    } else {
        qCFatal(lcCredentials) << "flatpak Secret Portal is not valid - bailing out";
    }
#else
    m_initialized = true;
    emit initializedChanged();
#endif
}

void Credentials::set(const QString &key, const QString &secret, CredentialsResponse callback)
{
    m_writeCredentialJob.setKey(key);

    connect(&m_writeCredentialJob, &QKeychain::WritePasswordJob::finished, this,
            [this, callback]() {
                if (m_writeCredentialJob.error()) {
                    callback(true,
                             tr("storing credentials failed: %1")
                                     .arg(qPrintable(m_writeCredentialJob.errorString())));
                } else {
                    callback(false, "");
                }
            });

    m_writeCredentialJob.setTextData(secret);
    m_writeCredentialJob.start();
}

void Credentials::get(const QString &key, CredentialsResponse callback)
{
    m_readCredentialJob.setKey(key);

    QObject::connect(&m_readCredentialJob, &QKeychain::ReadPasswordJob::finished, this,
                     [this, key, callback]() {
                         auto error = m_readCredentialJob.error();
                         QString secret = m_readCredentialJob.textData();

                         // Key is not present in keychain? Try to update it from Flatpak portal
                         if (error == QKeychain::EntryNotFound || secret.isEmpty()) {
                             KeychainSettings keychainSettings;

#ifdef Q_OS_LINUX
                             const auto encryptedSecret =
                                     keychainSettings.value(key, "").toString();
                             secret = SecretPortal::instance().decrypt(encryptedSecret);

                             if (!secret.isEmpty()) {

                                 set(key, secret, [key](bool error, const QString &misc) {
                                     if (error) {
                                         qCCritical(lcCredentials)
                                                 << "failed to update keychain credentials for"
                                                 << key << "-" << misc;
                                     }
                                 });
                             }
#endif

                             callback(false, secret);
                         } else if (error != QKeychain::NoError) {
                             callback(true,
                                      tr("reading credentials failed: %1")
                                              .arg(qPrintable(m_writeCredentialJob.errorString())));
                         } else {
                             callback(false, secret);
                         }
                     });

    m_readCredentialJob.start();
}
