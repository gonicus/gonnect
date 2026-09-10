#pragma once

#include <QString>
#include <QUrl>

struct IpcConfig
{
    enum class LoginFlow { Unknown, Credentials, SSO };

    QString configHash;
    QString persistentStorageSecret;
    QString encryptionSecret;
    QString secret;

    QString displayName;
    QString userId;
    QUrl backendUrl;
    QString identityProviderId;
    LoginFlow loginFlow = LoginFlow::Unknown;
    QString idConvRegexpString;
    QString idConvReplacementString;
};
