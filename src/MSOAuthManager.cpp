#include "MSOAuthManager.h"
#include "ReadOnlyConfdSettings.h"
#include "Credentials.h"
#include "ViewHelper.h"
#include "ErrorBus.h"
#include <QOAuth2AuthorizationCodeFlow>
#include <QOAuthHttpServerReplyHandler>
#include <QDesktopServices>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(msOAuthManager, "gonnect.app.MSOAuthManager")

MSOAuthManager::MSOAuthManager(QObject *parent)
    : QObject(parent), m_group(QStringLiteral("msoauth"))
{
}

/// @brief Start OAuth login with Microsoft server, or refresh a previously successful login
///
/// This function attempts to load a refresh token from a previous login and use it with the
/// Microsoft servers, or (on failure) starts displaying a UI to the user indicating that a login
/// is required and requests user interaction.
///
/// @param configId The settings group of the plugin that started the login. This is used to
///                 identify the response signal from the UI.
/// @param reason A user visible reason for why the user should login to their Microsoft account.
/// @param scopes The scopes to request in the OAuth request if login is needed.
///
void MSOAuthManager::refreshOrRequestOauthLogin(const QString &configId, const QString &reason,
                                                const QSet<QByteArray> &scopes)
{
    if (m_readRefreshTokenIsOngoing) {
        qCDebug(msOAuthManager) << "OAuth flow already ongoing. Ignoring new request.";
        return;
    }
    if (m_authCodeFlow) {
        // Check if flow is already ongoing - may happen if two plugins request login,
        // e.g. on startup.
        switch (m_authCodeFlow->status()) {
        case QAbstractOAuth::Status::TemporaryCredentialsReceived:
        case QAbstractOAuth::Status::RefreshingToken:
            qCDebug(msOAuthManager) << "OAuth flow already ongoing. Ignoring new request.";
            return;
        default:
            break;
        }
    }
    m_readRefreshTokenIsOngoing = true;
    acquireRefreshToken([this, configId, reason, scopes](const QString &token) {
        m_readRefreshTokenIsOngoing = false;
        if (!token.isEmpty()) {
            initAuthCodeFlow();
            m_authCodeFlow->setRefreshToken(token);
            m_authCodeFlow->refreshTokens();
        } else {
            qCDebug(msOAuthManager) << "refreshToken not available. Require login.";
            requestLoginInUi(configId, reason, scopes);
        }
    });
}

void MSOAuthManager::clearRefreshToken()
{
    Credentials::instance().set(
            m_group + "/refreshToken", QString(),
            [](QKeychain::Error error, const QString &, const QString &message) {
                if (error != QKeychain::NoError) {
                    ErrorBus::instance().error(
                            tr("Failed to clear refresh token for Microsoft login: %2")
                                    .arg(message));
                }
            });
}

void MSOAuthManager::requestLoginInUi(const QString &configId, const QString &reason,
                                      const QSet<QByteArray> &scopes)
{
    auto &viewHelper = ViewHelper::instance();

    // Remove any potentially existing connections
    QObject::disconnect(&viewHelper, nullptr, this, nullptr);

    QObject::connect(&viewHelper, &ViewHelper::oauthLoginStartResponded, this,
                     [configId, scopes, this](const QString &id) {
                         if (id == configId) {
                             authorize(scopes);
                         }
                     });
    QObject::connect(&viewHelper, &ViewHelper::oauthLoginCloseResponded, this,
                     [configId, this](const QString &id) {
                         if (id == configId) {
                             QObject::disconnect(&ViewHelper::instance(), nullptr, this, nullptr);
                         }
                     });
    m_requestConfigId = configId;
    viewHelper.requestOauthLogin(m_requestConfigId, reason);
}

void MSOAuthManager::authStatusChanged(QAbstractOAuth::Status status)
{
    qCDebug(msOAuthManager) << "Microsoft auth status changed:" << static_cast<int>(status);
    if (status == QAbstractOAuth::Status::Granted) {
        m_replyHandler->close();
        showOauthLoginStatus(tr("Login successful."), false);
        Q_EMIT loginSuccessful();
    }
}

void MSOAuthManager::initAuthCodeFlow()
{
    if (m_authCodeFlow) {
        return;
    }

    ReadOnlyConfdSettings settings;
    settings.beginGroup(m_group);
    // The identifier of the "GOnnect" app in the microsoft backend.
    // This should be a Gonicus registered app (see the Microsoft Entra admin center), with the
    // redirect urls (see below) and the supported scopes configured.
    // (A different identifier, registered by someone else, could also be used, if desired)
    const QString clientIdentifier = settings.value("clientIdentifier").toString();
    const QString authorizationUrl =
            settings.value("authorizationUrl",
                           QStringLiteral("https://login.microsoftonline.com/common/oauth2/v2.0/"
                                          "authorize"))
                    .toString();
    const QString tokenUrl =
            settings.value("authorizationUrl",
                           QStringLiteral(
                                   "https://login.microsoftonline.com/common/oauth2/v2.0/token"))
                    .toString();

    // For each port a redirect url must be registered in the microsoft portal for this app.
    // http://localhost:33221/Gonnect, ...
    const std::array<uint16_t, 3> availablePorts = { 33221, 34221, 33521 };

    if (!m_replyHandler) {
        // NOTE: Use QHostAddress::Any here.
        //       Qt transforms "QHostAddress::Any" (and AnyIPv4 and AnyIPv6) to "localhost",
        //       which is the only valid string atm.
        //       NOTE: Address mutch match the address in the microsoft portal exactly.
        m_replyHandler =
                new QOAuthHttpServerReplyHandler(QHostAddress::Any, availablePorts[0], this);
        //: This is text is displayed in the web browser after the user successfully logged in to the microsoft account.
        m_replyHandler->setCallbackText(
                tr("Login to the Microsoft account has been received by GOnnect. Gonnect will "
                   "continue the authorization process now. You can close this page now."));
        m_replyHandler->setCallbackPath(QStringLiteral("/Gonnect"));
    }

    if (!m_replyHandler->isListening()) {
        // try and hope that one of the available ports is available for listening.
        // pick the first available port.
        for (auto port : availablePorts) {
            if (m_replyHandler->listen(QHostAddress::Any, port)) {
                break;
            }
        }

        if (!m_replyHandler->isListening()) {
            qCCritical(msOAuthManager)
                    << "Failed to start QOAuthHttpServerReplyHandler. No port available?";
            showOauthLoginStatus(
                    tr("Failed to start login, the local system could not be set up to receive a "
                       "response. Try again later.\nIf the problem persists, try to close running "
                       "applications that reserve/block network ports."),
                    true);
            return;
        }
    }

    m_authCodeFlow = new QOAuth2AuthorizationCodeFlow(this);
    m_authCodeFlow->setReplyHandler(m_replyHandler);
    m_authCodeFlow->setAuthorizationUrl(authorizationUrl);
    m_authCodeFlow->setTokenUrl(tokenUrl);
    m_authCodeFlow->setClientIdentifier(clientIdentifier);
    m_authCodeFlow->setAutoRefresh(true);

    connect(m_authCodeFlow, &QOAuth2AuthorizationCodeFlow::statusChanged, this,
            &MSOAuthManager::authStatusChanged);
    connect(m_authCodeFlow, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser,
            &QDesktopServices::openUrl);
    connect(m_authCodeFlow, &QOAuth2AuthorizationCodeFlow::serverReportedErrorOccurred, this,
            [&](const QString &error, const QString &errorDescription, const QUrl &uri) {
                qCInfo(msOAuthManager) << "Microsoft auth serverReportedErrorOccurred:" << error
                                       << "description:" << errorDescription << "uri:" << uri;
                showOauthLoginStatus(tr("Login failed, the server reported an error:\n%1\nWith "
                                        "error description: %2")
                                             .arg(error)
                                             .arg(errorDescription),
                                     true);
            });
    connect(m_authCodeFlow, &QOAuth2AuthorizationCodeFlow::requestFailed, this,
            [&](const QAbstractOAuth::Error error) {
                qCInfo(msOAuthManager)
                        << "Microsoft auth requestFailed:" << static_cast<int>(error);
                switch (error) {
                case QAbstractOAuth::Error::NoError:
                    break;
                case QAbstractOAuth::Error::NetworkError:
                    showOauthLoginStatus(tr("Login failed due to a network error. Check your "
                                            "internet connectivity and try again."),
                                         true);
                    break;
                case QAbstractOAuth::Error::ServerError:
                    showOauthLoginStatus(
                            tr("Login failed because the Microsoft server returned an unexpected "
                               "or incorrect response. Please try again later."),
                            true);
                    break;
                case QAbstractOAuth::Error::OAuthTokenNotFoundError: // fall-through intended
                case QAbstractOAuth::Error::OAuthTokenSecretNotFoundError:
                    showOauthLoginStatus(tr("Login failed, no token has been received."), true);
                    break;
                case QAbstractOAuth::Error::OAuthCallbackNotVerified:
                    showOauthLoginStatus(
                            tr("Login failed, possibly due to a server configuration error."),
                            true);
                    break;
                case QAbstractOAuth::Error::ClientError:
                    showOauthLoginStatus(tr("Login failed, possibly due to a GOnnect configuration "
                                            "error. Please contact the GOnnect support."),
                                         true);
                    break;
                case QAbstractOAuth::Error::ExpiredError:
                    showOauthLoginStatus(tr("Login failed, token expired. Please try again."),
                                         true);
                    break;
                default:
                    showOauthLoginStatus(tr("Login failed, unknown error. Please try again."),
                                         true);
                    break;
                }
            });
    connect(m_authCodeFlow, &QOAuth2AuthorizationCodeFlow::refreshTokenChanged, this,
            [this](const QString &refreshToken) {
                if (!refreshToken.isEmpty()) {
                    storeRefreshToken(refreshToken);
                }
            });

    settings.endGroup();
}

void MSOAuthManager::authorize(const QSet<QByteArray> &scopes)
{
    initAuthCodeFlow();
    if (!m_authCodeFlow) {
        return;
    }
    QString scopesString;
    for (auto s : scopes) {
        if (!scopesString.isEmpty()) {
            scopesString += ",";
        }
        scopesString += s;
    }
    qCDebug(msOAuthManager) << "Starting microsoft authorization for scopes " << scopesString;
    m_authCodeFlow->setRequestedScopeTokens(scopes);
    m_authCodeFlow->grant();
}

bool MSOAuthManager::isGranted() const
{
    if (!m_authCodeFlow) {
        return false;
    }
    return (m_authCodeFlow->status() == QAbstractOAuth::Status::Granted);
}

QString MSOAuthManager::token() const
{
    if (!m_authCodeFlow) {
        return QString();
    }
    return m_authCodeFlow->token();
}

void MSOAuthManager::acquireRefreshToken(std::function<void(const QString &token)> callback)
{
    Credentials::instance().get(
            m_group + "/refreshToken",
            [this, callback](QKeychain::Error error, const QString &secret, const QString &) {
                if (error == QKeychain::NoError) {
                    if (!secret.isEmpty()) {
                        qCDebug(msOAuthManager) << "refreshToken available for" << m_group;
                    } else {
                        qCDebug(msOAuthManager) << "refreshToken not available for" << m_group;
                    }
                } else {
                    if (error == QKeychain::EntryNotFound) {
                        qCDebug(msOAuthManager) << "No refreshToken found for" << m_group;
                    } else {
                        qCDebug(msOAuthManager)
                                << "Error reading refreshToken for" << m_group << ":" << error;
                    }
                }
                callback(secret);
            });
}

void MSOAuthManager::storeRefreshToken(const QString &token) const
{
    Credentials::instance().set(
            m_group + "/refreshToken", token,
            [](QKeychain::Error error, const QString &, const QString &message) {
                if (error != QKeychain::NoError) {
                    ErrorBus::instance().error(
                            tr("Failed to persist refresh token for Microsoft login: %2")
                                    .arg(message));
                }
            });
}

void MSOAuthManager::showOauthLoginStatus(const QString &status, bool canRetry)
{
    auto &viewHelper = ViewHelper::instance();
    viewHelper.showOauthLoginStatus(m_requestConfigId, status, canRetry);
}
