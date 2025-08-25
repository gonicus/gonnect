#include "AuthManager.h"
#include "ReadOnlyConfdSettings.h"
#include "Credentials.h"

#include <QtNetworkAuth/QtNetworkAuth>
#include <QLoggingCategory>
#include <QDesktopServices>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>

Q_LOGGING_CATEGORY(lcAuthManager, "gonnect.app.auth")

AuthManager::AuthManager(QObject *parent) : QObject{ parent }
{
    init();
}

void AuthManager::init()
{
    m_isAuthManagerInitialized = true;

    ReadOnlyConfdSettings settings;
    settings.beginGroup("jitsi");

    // Use this when other auth types for Jitsi Meet are implemented
    m_isJitsiAuthRequired = settings.value("authorizationMode", "none").toString() != "none";
    if (!m_isJitsiAuthRequired) {
        emit isAuthManagerInitializedChanged();
        return;
    }

    QSslConfiguration sslConfig;
    sslConfig.setPeerVerifyMode(QSslSocket::PeerVerifyMode::VerifyNone);
    sslConfig.addCaCertificates(sslCAs());
    m_reqFactory.setSslConfiguration(sslConfig);

    m_reqFactory.setBaseUrl(settings.value("baseUrl", "").toUrl());

    m_authFlow = new QOAuth2AuthorizationCodeFlow(this);

    QObject::connect(m_authFlow, &QOAuth2AuthorizationCodeFlow::serverReportedErrorOccurred, this,
                     [](const QString &error, const QString &errorDescription, const QUrl &url) {
                         qCCritical(lcAuthManager)
                                 << "OAuth error:" << error << errorDescription << url;
                     });

    QObject::connect(m_authFlow, &QOAuth2AuthorizationCodeFlow::tokenChanged, this,
                     [this](const QString &token) {
                         qCInfo(lcAuthManager)
                                 << "Received new OAuth access token - updating bearer";
                         m_reqFactory.setBearerToken(token.toLatin1());

                         if (isOAuthAuthenticated()) {
                             m_oauthToken = token;
                             emit oAuthReady();
                         }
                     });

    QObject::connect(m_authFlow, &QOAuth2AuthorizationCodeFlow::refreshTokenChanged, this,
                     [this](const QString &refreshToken) { storeRefreshToken(refreshToken); });

    QObject::connect(m_authFlow, &QOAuth2AuthorizationCodeFlow::expirationAtChanged, this,
                     [this](const QDateTime &) {
                         if (isOAuthAuthenticated()) {
                             m_oauthToken.clear();
                             emit oAuthReady();
                         }
                     });

    QObject::connect(m_authFlow, &QOAuth2AuthorizationCodeFlow::serverReportedErrorOccurred, this,
                     [](const QString &error, const QString &errorDescription, const QUrl &uri) {
                         qCInfo(lcAuthManager).nospace().noquote()
                                 << "OAuth error reported by server: " << error << " ("
                                 << errorDescription << ", " << uri << ")";
                     });

    QObject::connect(m_authFlow, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, this,
                     [](const QUrl &url) {
                         qCInfo(lcAuthManager) << "Opening browser for auth:" << url;
                         const bool result = QDesktopServices::openUrl(url);
                         qCInfo(lcAuthManager) << "Browser opened:" << result;
                     });

    Credentials::instance().get("jitsi/refreshToken", [this, sslConfig](bool error, const QString &data) {
        if (error) {
            qCCritical(lcAuthManager) << "failed to set credentials:" << data;
            return;
        }

        if (!data.isEmpty()) {
            m_authFlow->setRefreshToken(data);
        }

        ReadOnlyConfdSettings settings;
        settings.beginGroup("jitsi");

        m_authFlow->setAutoRefresh(true);
        m_authFlow->setAuthorizationUrl(settings.value("authorizationUrl", "").toUrl());
        m_authFlow->setTokenUrl(settings.value("tokenUrl", "").toUrl());
        m_authFlow->setClientIdentifier(settings.value("clientIdentifier", "").toString());
        m_authFlow->setSslConfiguration(sslConfig);

        settings.endGroup();

        emit isAuthManagerInitializedChanged();
    });
}

void AuthManager::storeRefreshToken(const QString &token) const
{
    if (token.isEmpty()) {
        return;
    }

    Credentials::instance().set("jitsi/refreshToken", token, [](bool error, const QString &data) {
        if (error) {
            qCCritical(lcAuthManager) << "failed to set credentials:" << data;
        }
    });
}

bool AuthManager::isOAuthAuthenticated() const
{
    if (!m_authFlow) {
        return false;
    }
    const auto expiration = m_authFlow->expirationAt();
    return expiration.isValid() && QDateTime::currentDateTime() < expiration;
}

bool AuthManager::ensureOAuthAuthenticated()
{
    if (isOAuthAuthenticated()) {
        return true;
    }

    if (!m_authFlow->refreshToken().isEmpty()) {
        const auto expirationDate = tokenExpiry(m_authFlow->refreshToken());

        if (expirationDate.isValid()
            && QDateTime::currentDateTime() < expirationDate.addSecs(-60)) {
            m_authFlow->refreshTokens();
            return false;
        }
    }

    auto replyHandler = new QOAuthHttpServerReplyHandler(this);

    QObject::connect(replyHandler, &QOAuthHttpServerReplyHandler::tokenRequestErrorOccurred, this,
                     [replyHandler](QAbstractOAuth::Error error, const QString &errorString) {
                         Q_UNUSED(error);
                         qCCritical(lcAuthManager) << "Error on receiving token:" << errorString;
                         replyHandler->close();
                         replyHandler->deleteLater();
                     });

    QObject::connect(replyHandler, &QOAuthHttpServerReplyHandler::tokensReceived, this,
                     [replyHandler](const QVariantMap &) {
                         replyHandler->close();
                         replyHandler->deleteLater();
                     });

    m_authFlow->setReplyHandler(replyHandler);
    m_authFlow->grant();

    return false;
}

void AuthManager::setIsWaitingForAuth(bool value)
{
    if (m_isWaitingForAuth != value) {
        m_isWaitingForAuth = value;
        emit isWaitingForAuthChanged();
    }
}

QDateTime AuthManager::tokenExpiry(const QString &token) const
{
    if (token.isEmpty()) {
        qCWarning(lcAuthManager) << "Cannot parse expiration date of empty string";
        return QDateTime();
    }

    const auto parts = token.split('.');
    if (parts.size() != 3) {
        qCCritical(lcAuthManager)
                << "Error parsing refreh token: expected to be three parts in token, but found"
                << parts.size();
        return QDateTime();
    }

    QJsonParseError err;
    const auto payload =
            QJsonDocument::fromJson(QByteArray::fromBase64(parts.at(1).toUtf8()), &err);

    if (payload.isNull()) {
        qCCritical(lcAuthManager) << "Error parsing refresh token:" << err.errorString();
        return QDateTime();
    }

    if (!payload.isObject()) {
        qCCritical(lcAuthManager) << "Error parsing refresh token: JSON structure is not an object";
        return QDateTime();
    }

    const auto obj = payload.object();

    if (!obj.contains("exp")) {
        qCCritical(lcAuthManager) << "Error token does not contain expiration date";
        return QDateTime();
    }

    const auto expValue = obj.value("exp");
    if (!expValue.isDouble()) {
        qCCritical(lcAuthManager) << "Expiration date has invalid data type";
        return QDateTime();
    }

    return QDateTime::fromSecsSinceEpoch(expValue.toInt());
}

void AuthManager::authenticateJitsiRoom(const QString &roomName)
{
    if (!roomName.isEmpty()) {
        if (ensureOAuthAuthenticated()) {
            authenticateJitsiImpl(roomName);
        } else {
            setIsWaitingForAuth(true);

            if (m_authWaitingContext) {
                delete m_authWaitingContext;
                m_authWaitingContext = nullptr;
            }

            m_authWaitingContext = new QObject(this);
            m_jitsiRoomWaitingForAuth = roomName;

            connect(this, &AuthManager::oAuthReady, m_authWaitingContext,
                    [this]() { authRoomWaiting(); });
        }
    }
}

bool AuthManager::isJitsiRoomAuthenticated(const QString &roomName) const
{
    return m_jitsiTokens.contains(roomName)
            && QDateTime::currentDateTime() < m_jitsiTokens.value(roomName).expires;
}

QByteArray AuthManager::jitsiTokenForRoom(const QString &roomName) const
{
    if (m_jitsiTokens.contains(roomName)) {
        const auto &token = m_jitsiTokens.value(roomName);
        if (QDateTime::currentDateTime() < token.expires) {
            return token.token;
        }
    }
    return "";
}

QString AuthManager::userUid() const
{
    return qgetenv("LOGNAME");
}

const QList<QSslCertificate> &AuthManager::sslCAs()
{
    if (!m_isCAsInitialized) {
        m_isCAsInitialized = true;

        ReadOnlyConfdSettings settings;
        const auto pathList = settings.value("generic/caFiles").toStringList();

        for (const auto &path : pathList) {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                qCCritical(lcAuthManager) << "Unable to read CA file" << path;
                continue;
            }

            QSslCertificate cert(file.readAll());
            file.close();

            m_cas.append(cert);
        }
    }
    return m_cas;
}

void AuthManager::authenticateJitsiImpl(const QString &roomName)
{
    setIsWaitingForAuth(true);

    Q_ASSERT(isOAuthAuthenticated());

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QObject::connect(manager, &QNetworkAccessManager::finished, this,
                     [this, roomName](QNetworkReply *reply) {
                         QJsonParseError errObj;
                         const auto document = QJsonDocument::fromJson(reply->readAll(), &errObj);

                         if (document.isNull()) {
                             qCCritical(lcAuthManager)
                                     << "Error while parsing json:" << errObj.errorString();
                             setIsWaitingForAuth(false);
                             return;
                         }
                         if (!document.isObject()) {
                             qCCritical(lcAuthManager) << "Error: json ist not an object";
                             setIsWaitingForAuth(false);
                             return;
                         }
                         const auto jsonObj = document.object();
                         if (!jsonObj.contains("token")) {
                             qCCritical(lcAuthManager) << "Error: json is missing key 'token'";
                             setIsWaitingForAuth(false);
                             return;
                         }

                         // Decode JWT
                         const auto encodedJwt = jsonObj.value("token").toString().toLatin1();
                         const auto decodedJwt =
                                 QByteArray::fromBase64(encodedJwt.split('.').at(1));
                         const auto jwtJsonObject =
                                 QJsonDocument::fromJson(decodedJwt).object().toVariantHash();

                         if (!jwtJsonObject.contains("room")) {
                             qCCritical(lcAuthManager) << "Error: json is missing key 'room'";
                             setIsWaitingForAuth(false);
                             return;
                         }
                         if (!jwtJsonObject.contains("exp")) {
                             qCCritical(lcAuthManager) << "Error: json is missing key 'exp'";
                             setIsWaitingForAuth(false);
                             return;
                         }

                         bool parsed = false;
                         const auto expireDate = QDateTime::fromSecsSinceEpoch(
                                 jwtJsonObject.value("exp").toUInt(&parsed));
                         if (!parsed || !expireDate.isValid()) {
                             qCCritical(lcAuthManager) << "Error: json value for 'exp' is invalid";
                             setIsWaitingForAuth(false);
                             return;
                         }

                         const auto roomName = jwtJsonObject.value("room").toString();
                         Token token(expireDate, encodedJwt);
                         m_jitsiTokens.insert(roomName, token);

                         qCInfo(lcAuthManager) << "Successfully received JWT for room" << roomName;
                         setIsWaitingForAuth(false);
                         emit jitsiRoomAuthenticated(roomName);
                     });

    auto request = m_reqFactory.createRequest(QUrlQuery(QString("room=%1").arg(roomName)));

    QSslConfiguration sslConfig;
    sslConfig.setPeerVerifyMode(QSslSocket::PeerVerifyMode::VerifyNone);
    sslConfig.addCaCertificates(sslCAs());
    request.setSslConfiguration(sslConfig);

    QNetworkReply *reply = manager->get(request);

    QObject::connect(
            reply, &QNetworkReply::errorOccurred, this,
            [this, reply](QNetworkReply::NetworkError code) {
                qCCritical(lcAuthManager)
                        << "Network error occurred while receiving Jitsi jwt:" << code
                        << "http code:"
                        << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                setIsWaitingForAuth(false);
            });
    QObject::connect(
            reply, &QNetworkReply::sslErrors, this, [this](const QList<QSslError> &errors) {
                qCCritical(lcAuthManager) << "SSL error(s) occurred while receiving jwt:" << errors;

                for (const auto &err : errors) {
                    qCCritical(lcAuthManager()) << err << err.errorString();
                }

                setIsWaitingForAuth(false);
            });
}

void AuthManager::authRoomWaiting()
{
    if (isOAuthAuthenticated()) {
        delete m_authWaitingContext;
        m_authWaitingContext = nullptr;

        authenticateJitsiImpl(m_jitsiRoomWaitingForAuth);
        m_jitsiRoomWaitingForAuth = "";
    }
}
