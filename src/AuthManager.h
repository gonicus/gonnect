#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QNetworkRequestFactory>
#include <QSslCertificate>

class QOAuth2AuthorizationCodeFlow;

class AuthManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isAuthManagerInitialized READ isAuthManagerInitialized NOTIFY
                       isAuthManagerInitializedChanged FINAL)
    Q_PROPERTY(bool isJitsiAuthRequired READ isJitsiAuthRequired NOTIFY
                       isAuthManagerInitializedChanged FINAL)
    Q_PROPERTY(bool isWaitingForAuth READ isWaitingForAuth NOTIFY isWaitingForAuthChanged FINAL)
    Q_PROPERTY(bool isOAuthAuthenticated READ isOAuthAuthenticated NOTIFY oAuthReady FINAL)

private:
    struct Token
    {
        QDateTime expires;
        QByteArray token;
    };

public:
    static AuthManager &instance()
    {
        static AuthManager *_instance = nullptr;
        if (!_instance) {
            _instance = new AuthManager;
        }
        return *_instance;
    }

    bool isAuthManagerInitialized() const { return m_isAuthManagerInitialized; }
    bool isJitsiAuthRequired() const { return m_isJitsiAuthRequired; }

    Q_INVOKABLE void authenticateJitsiRoom(const QString &roomName);

    /// Whether the AuthManager currently has a valid, non-expired token for the given Jitsi
    /// room. If not, it must be requested via authenticateJitsiRoom().
    Q_INVOKABLE bool isJitsiRoomAuthenticated(const QString &roomName) const;

    /// Receive the JWT for the stated room. If result is empty, the token has not (yet) been
    /// received or is expired and caller must wait for the jitsiRoomAuthenticated signal to be
    /// sent and then try again.
    QByteArray jitsiTokenForRoom(const QString &roomName) const;

    /// Returns true if currently authenticated. If not, it returns false and starts the auth
    /// process. Watch isOAuthAuthenticated() (oAuthReady() resp.) to know when that process is
    /// completed.
    bool ensureOAuthAuthenticated();

    /// The current OAuth token or an empty string if there is no valid token
    QString oauthToken() const { return m_oauthToken; }

    /// Whether the OAuth process has been successfully completed and a non-expired access token
    /// exists.
    bool isOAuthAuthenticated() const;

    /// The uid of the current user.
    Q_INVOKABLE QString userUid() const;

    /// Whether the authentication process has started and it is waited for it to finish
    bool isWaitingForAuth() const { return m_isWaitingForAuth; }

    /// Get a list of the configured CA certificates; empty when none are configured or could be
    /// loaded
    const QList<QSslCertificate> &sslCAs();

private slots:
    void authenticateJitsiImpl(const QString &roomName);
    void authRoomWaiting();

private:
    explicit AuthManager(QObject *parent = nullptr);

    void init();
    void storeRefreshToken(const QString &token) const;
    void setIsWaitingForAuth(bool value);
    QDateTime tokenExpiry(const QString &token) const;

    bool m_isAuthManagerInitialized = false;
    bool m_isJitsiAuthRequired = false;
    bool m_isWaitingForAuth = false;
    QString m_oauthToken;
    QString m_jitsiRoomWaitingForAuth;
    QHash<QString, Token> m_jitsiTokens;
    bool m_isCAsInitialized = false;
    QList<QSslCertificate> m_cas;

    QDateTime m_expireDateTime;
    QOAuth2AuthorizationCodeFlow *m_authFlow = nullptr;
    QObject *m_authWaitingContext = nullptr;
    QNetworkRequestFactory m_reqFactory;

signals:
    void isAuthManagerInitializedChanged();
    void oAuthReady();
    void jitsiRoomAuthenticated(QString roomName);
    void authenticatedJitsiRoomChanged();
    void isWaitingForAuthChanged();
};

class AuthManagerWrapper
{
    Q_GADGET
    QML_FOREIGN(AuthManager)
    QML_NAMED_ELEMENT(AuthManager)
    QML_SINGLETON

public:
    static AuthManager *create(QQmlEngine *, QJSEngine *) { return &AuthManager::instance(); }

private:
    AuthManagerWrapper() = default;
};
