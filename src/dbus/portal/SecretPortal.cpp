#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "SecretPortal.h"
#include "AppSettings.h"
#include "SecretGenerator.h"

Q_LOGGING_CATEGORY(lcSecretPortal, "gonnect.secrets")

static constexpr int AES_BLOCK_SIZE = 16;
static constexpr int AES_256_KEY_LEN = 32;

struct EvpCtxDeleter
{
    void operator()(EVP_CIPHER_CTX *ctx) const { EVP_CIPHER_CTX_free(ctx); }
};
using EvpCtxPtr = std::unique_ptr<EVP_CIPHER_CTX, EvpCtxDeleter>;

static QByteArray opensslEncrypt(const QByteArray &plainText, const QByteArray &key,
                                 const QByteArray &iv)
{
    if (key.size() != AES_256_KEY_LEN || iv.size() != AES_BLOCK_SIZE) {
        return {};
    }

    EvpCtxPtr ctx(EVP_CIPHER_CTX_new());
    if (!ctx) {
        return {};
    }

    if (EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_cbc(), nullptr,
                           reinterpret_cast<const unsigned char *>(key.constData()),
                           reinterpret_cast<const unsigned char *>(iv.constData()))
        != 1) {
        return {};
    }

    QByteArray cipherText(plainText.size() + AES_BLOCK_SIZE, '\0');
    int outLen1 = 0;
    int outLen2 = 0;

    if (EVP_EncryptUpdate(ctx.get(), reinterpret_cast<unsigned char *>(cipherText.data()), &outLen1,
                          reinterpret_cast<const unsigned char *>(plainText.constData()),
                          plainText.size())
        != 1) {
        return {};
    }

    if (EVP_EncryptFinal_ex(
                ctx.get(), reinterpret_cast<unsigned char *>(cipherText.data()) + outLen1, &outLen2)
        != 1) {
        return {};
    }

    cipherText.resize(outLen1 + outLen2);
    return cipherText;
}

static QByteArray opensslDecrypt(const QByteArray &cipherText, const QByteArray &key,
                                 const QByteArray &iv)
{
    if (key.size() != AES_256_KEY_LEN || iv.size() != AES_BLOCK_SIZE || cipherText.isEmpty()) {
        return {};
    }

    EvpCtxPtr ctx(EVP_CIPHER_CTX_new());
    if (!ctx) {
        return {};
    }

    if (EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_cbc(), nullptr,
                           reinterpret_cast<const unsigned char *>(key.constData()),
                           reinterpret_cast<const unsigned char *>(iv.constData()))
        != 1) {
        return {};
    }

    QByteArray plainText(cipherText.size() + AES_BLOCK_SIZE, '\0');
    int outLen1 = 0;
    int outLen2 = 0;

    if (EVP_DecryptUpdate(ctx.get(), reinterpret_cast<unsigned char *>(plainText.data()), &outLen1,
                          reinterpret_cast<const unsigned char *>(cipherText.constData()),
                          cipherText.size())
        != 1) {
        return {};
    }

    if (EVP_DecryptFinal_ex(ctx.get(),
                            reinterpret_cast<unsigned char *>(plainText.data()) + outLen1, &outLen2)
        != 1) {
        return {};
    }

    plainText.resize(outLen1 + outLen2);
    return plainText;
}

SecretPortal::SecretPortal(QObject *parent)
    : AbstractPortal{ FREEDESKTOP_DBUS_PORTAL_SERVICE, FREEDESKTOP_DBUS_PORTAL_PATH,
                      SECRET_PORTAL_INTERFACE, parent }
{
}

void SecretPortal::initialize()
{
    if (!isValid()) {
        m_hasTriedInitialization = true;
        return;
    }

    // Verify that AES-256-CBC is available in the linked OpenSSL build.
    // EVP_aes_256_cbc() returns a non-null pointer when supported.
    m_supported = (EVP_aes_256_cbc() != nullptr);

    if (!m_supported) {
        m_hasTriedInitialization = true;
        qCFatal(lcSecretPortal) << "OpenSSL does not support AES-256-CBC!";
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

    AppSettings settings;

    // For backwards compatibility, fall back to the stored iv value that is
    // stored in the configuration. This is not ideal, as iv should be stored
    // with the secret instead, but changing it would break with old versions.
    // Also note that the secret portal is a fallback for old 1.x versions
    // anyway.
    if (!settings.contains("keychain/iv")) {
        unsigned char ivBuf[AES_BLOCK_SIZE];
        if (RAND_bytes(ivBuf, AES_BLOCK_SIZE) != 1) {
            qCFatal(lcSecretPortal) << "RAND_bytes failed while generating IV";
        }

        settings.setValue(
                "keychain/iv",
                QString::fromLatin1(
                        QByteArray(reinterpret_cast<const char *>(ivBuf), AES_BLOCK_SIZE).toHex()));

        m_iv = QByteArray::fromHex(settings.value("keychain/iv").toByteArray());
    }

    QByteArray key = m_instanceSecret.leftJustified(AES_256_KEY_LEN, '\0').left(AES_256_KEY_LEN);
    QByteArray iv = m_iv.leftJustified(AES_BLOCK_SIZE, '\0').left(AES_BLOCK_SIZE);

    const QByteArray cipherBytes = opensslEncrypt(plainText.toUtf8(), key, iv);
    if (cipherBytes.isEmpty()) {
        qCCritical(lcSecretPortal) << "encryption failed";
        return "";
    }

    return QString::fromLatin1(cipherBytes.toHex());
}

QString SecretPortal::decrypt(const QString &cipherText)
{
    if (!m_supported || !m_initialized || m_iv.isEmpty()) {
        qCCritical(lcSecretPortal) << "secret portal is not available";
        return "";
    }

    QByteArray key = m_instanceSecret.leftJustified(AES_256_KEY_LEN, '\0').left(AES_256_KEY_LEN);
    QByteArray iv = m_iv.leftJustified(AES_BLOCK_SIZE, '\0').left(AES_BLOCK_SIZE);
    QByteArray cipherBytes = QByteArray::fromHex(cipherText.toLatin1());

    const QByteArray plainBytes = opensslDecrypt(cipherBytes, key, iv);
    if (plainBytes.isEmpty()) {
        qCCritical(lcSecretPortal) << "decryption failed";
        return "";
    }

    return QString::fromUtf8(plainBytes);
}
