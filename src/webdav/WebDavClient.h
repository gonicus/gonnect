#pragma once

#include "WebDavPropertyRequest.h"
#include "WebDavPropfindParser.h"
#include "WebDavSyncChanges.h"

#include <QNetworkAccessManager>
#include <QObject>
#include <QCoroCore>
#include <QCoroNetwork>

class WebDavClient : public QObject
{
    Q_OBJECT

public:
    struct ConnectionConfig
    {
        QUrl url;
        QString username;
        QString password;
    };

    explicit WebDavClient(const ConnectionConfig &config, QObject *parent = nullptr);

    /// Send a WebDAV request. Caller takes ownership of returned QNetworkReply* and must destroy it
    /// afterwards.
    [[nodiscard]] QCoro::Task<QNetworkReply *>
    sendRequest(const QString &relativePath, const QByteArray &verb, const QByteArray &body);

    [[nodiscard]] QCoro::Task<QNetworkReply *>
    propfind(const QString &path, int depth, const QList<WebDavPropertyRequest> &properties);

    [[nodiscard]] QCoro::Task<QNetworkReply *> report(const QString &path, const QByteArray &body,
                                                      int depth = 0);

    /// Changes via sync-collection. Empty token requests all (initial sync).
    [[nodiscard]] QCoro::Task<WebDavSyncChanges> syncCollection(const QString &path,
                                                                QString syncToken);

private Q_SLOTS:
    void onAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);

private:
    void initConnection();
    QNetworkRequest createNetworkRequest(const QString &relativePath) const;

    QNetworkAccessManager m_networkAccessManager;
    ConnectionConfig m_connectionConfig;
    QSslConfiguration m_sslConfiguration;
    WebDavPropfindParser m_propfindParser;
};
