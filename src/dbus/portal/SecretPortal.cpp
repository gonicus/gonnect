#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <QtCrypto>

#include "SecretPortal.h"
#include "AppSettings.h"

Q_LOGGING_CATEGORY(lcSecretPortal, "gonnect.secrets")

SecretPortal::SecretPortal(QObject *parent)
    : AbstractPortal{ FREEDESKTOP_DBUS_PORTAL_SERVICE, FREEDESKTOP_DBUS_PORTAL_PATH,
                      SECRET_PORTAL_INTERFACE, parent }
{
    const QString chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const auto charsSize = chars.length();

    AppSettings settings;
    if (!settings.contains("keychain/iv")) {
        QString randomString;
        for (int i = 0; i < 16; i++) {
            auto r = QRandomGenerator::global()->generate();
            randomString.append(chars.at(r % charsSize));
        }

        settings.setValue("keychain/iv", randomString);
    }

    m_iva = settings.value("keychain/iv").toByteArray();
}

void SecretPortal::initialize()
{
    if (!isValid()) {
        m_hasTriedInitialization = true;
        return;
    }

    QCA::Initializer init;
    m_supported = QCA::isSupported("aes256-cbc-pkcs7");

    if (!m_supported) {
        m_hasTriedInitialization = true;
        qCFatal(lcSecretPortal) << "QCA does not support aes256-cbc-pkcs7!";
        return;
    }

    RetrieveSecret([this](uint code, const QVariantMap &response) {
        m_hasTriedInitialization = true;

        if (code == 0) {
            m_instanceSecret = response.value("secret").toByteArray();
            m_initialized = true;
        } else {
            const auto errorMsg = response.value("error").toString();

            if (!errorMsg.isEmpty()) {
                qCCritical(lcSecretPortal).noquote().nospace()
                        << "failed to retrieve instance secret, error: " << errorMsg
                        << " (error code " << code << ")";
            } else {
                qCCritical(lcSecretPortal).noquote().nospace()
                        << "failed to retrieve instance secret (error code " << code << ")";
            }
        }

        Q_EMIT initializedChanged();
    });
}

void SecretPortal::RetrieveSecret(PortalResponse callback)
{
    QCA::Initializer init;

    QDBusMessage message = QDBusMessage::createMethodCall(
            FREEDESKTOP_DBUS_PORTAL_SERVICE, FREEDESKTOP_DBUS_PORTAL_PATH, SECRET_PORTAL_INTERFACE,
            "RetrieveSecret");

    QString token = generateHandleToken();

    QVariantMap options = { { "handle_token", QVariant(token) } };

    if (pipe(m_fds) < 0) {
        callback(1, { { "error", "failed to open secret pipe" } });

        return;
    }

    message << QVariant::fromValue(QDBusUnixFileDescriptor(m_fds[1])) << options;

    portalCall(token, message, [this, callback](uint code, const QVariantMap &response) {
        char secret[SECRET_MAX_LEN];
        fd_set set;
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000;

        FD_ZERO(&set);
        FD_SET(m_fds[0], &set);

        int rv = select(m_fds[0] + 1, &set, NULL, NULL, &timeout);
        if (rv == -1) {
            callback(
                    2,
                    { { "error",
                        "failed to select from secrets pipe: " + QString(std::strerror(errno)) } });
        } else if (rv == 0) {
            callback(2, { { "error", "timeout while reading secrets pipe" } });
        } else {
            size_t count = read(m_fds[0], secret, SECRET_MAX_LEN);
            if (count < SECRET_MAX_LEN) {
                QVariantMap reply = response;
                reply.insert("secret", QByteArray(secret, count));

                callback(code, reply);
            } else {
                callback(2, { { "error", "secret size too large" } });
            }
        }

        close(m_fds[0]);
        close(m_fds[1]);
    });
}

QString SecretPortal::encrypt(const QString &plainText)
{
    if (!m_supported || !m_initialized) {
        qCCritical(lcSecretPortal) << "secret portal is not available";
        return "";
    }

    QCA::Initializer init;
    QCA::SecureArray arg = plainText.toUtf8();
    QCA::InitializationVector iv(m_iva);

    QCA::Cipher cipher(QStringLiteral("aes256"), QCA::Cipher::CBC, QCA::Cipher::DefaultPadding,
                       QCA::Encode, m_instanceSecret, iv);

    QCA::SecureArray u = cipher.update(arg);
    if (!cipher.ok()) {
        qCCritical(lcSecretPortal) << "cipher update failed";
        return "";
    }

    QCA::SecureArray f = cipher.final();
    if (!cipher.ok()) {
        qCCritical(lcSecretPortal) << "cipher finalization failed";
        return "";
    }

    QCA::SecureArray cipherText = u.append(f);
    return QCA::arrayToHex(cipherText.toByteArray());
}

QString SecretPortal::decrypt(const QString &cipherText)
{
    if (!m_supported || !m_initialized) {
        qCCritical(lcSecretPortal) << "secret portal is not available";
        return "";
    }

    QCA::Initializer init;
    QCA::SecureArray arg = QCA::hexToArray(cipherText);
    QCA::InitializationVector iv(m_iva);

    QCA::Cipher cipher(QStringLiteral("aes256"), QCA::Cipher::CBC, QCA::Cipher::DefaultPadding,
                       QCA::Decode, m_instanceSecret, iv);

    return QCA::SecureArray(cipher.process(arg)).data();
}
