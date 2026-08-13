#pragma once

#include <QObject>
#include <QAbstractOAuth>

class QOAuth2AuthorizationCodeFlow;
class QOAuthHttpServerReplyHandler;

/// Helper class to perform an OAuth2 login for the user to the user's Microsoft account.
/// The login can then be used to use Microsoft APIs such as Microsoft Graph to access contacts
/// and calendars.
///
/// See also https://learn.microsoft.com/en-us/entra/identity-platform/ and
/// https://learn.microsoft.com/en-us/graph/auth/
class MSOAuthManager : public QObject
{
    Q_OBJECT

public:
    static MSOAuthManager &instance()
    {
        static MSOAuthManager *_instance = nullptr;
        if (!_instance) {
            _instance = new MSOAuthManager;
        }
        return *_instance;
    }

    void refreshOrRequestOauthLogin(const QString &configId, const QString &reason,
                                    const QSet<QByteArray> &scopes);
    void clearRefreshToken();

    bool isGranted() const;
    QString token() const;

Q_SIGNALS:
    void loginSuccessful();

private:
    explicit MSOAuthManager(QObject *parent = nullptr);
    void acquireRefreshToken(std::function<void(const QString &token)> callback);
    void initAuthCodeFlow();
    void requestLoginInUi(const QString &configId, const QString &reason,
                          const QSet<QByteArray> &scopes);
    void authStatusChanged(QAbstractOAuth::Status status);
    void showOauthLoginStatus(const QString &status, bool canRetry);
    void storeRefreshToken(const QString &token) const;
    void authorize(const QSet<QByteArray> &scopes);

    const QString m_group;
    QOAuthHttpServerReplyHandler *m_replyHandler = nullptr;
    QOAuth2AuthorizationCodeFlow *m_authCodeFlow = nullptr;
    QString m_requestConfigId;
    bool m_readRefreshTokenIsOngoing = false;
};
