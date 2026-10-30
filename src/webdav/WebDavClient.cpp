#include "WebDavClient.h"
#include "CertHelper.h"
#include "ReadOnlyConfdSettings.h"

#include <QAuthenticator>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcWebDavClient, "gonnect.app.webdav.WebDavClient")

WebDavClient::WebDavClient(const ConnectionConfig &config, QObject *parent)
    : QObject{ parent }, m_connectionConfig{ config }
{
    connect(&m_networkAccessManager, &QNetworkAccessManager::authenticationRequired, this,
            &WebDavClient::onAuthenticationRequired);

    initConnection();
}

void WebDavClient::initConnection()
{
    ReadOnlyConfdSettings settings;

    m_sslConfiguration = QSslConfiguration::defaultConfiguration();
    if (!settings.value("generic/verifyServer", true).toBool()) {
        m_sslConfiguration.setPeerVerifyMode(QSslSocket::PeerVerifyMode::VerifyNone);
    }
    m_sslConfiguration.addCaCertificates(CertHelper::instance().caCerts());
}

QNetworkRequest WebDavClient::createNetworkRequest(const QString &relativePath) const
{
    QNetworkRequest request(m_connectionConfig.url.resolved(relativePath));
    request.setSslConfiguration(m_sslConfiguration);
    return request;
}

QCoro::Task<QNetworkReply *> WebDavClient::sendRequest(const QString &relativePath,
                                                       const QByteArray &verb,
                                                       const QByteArray &body)
{
    auto request = createNetworkRequest(relativePath);

    QNetworkReply *reply = (verb == "GET")
            ? m_networkAccessManager.get(request)
            : m_networkAccessManager.sendCustomRequest(request, verb, body);
    co_await reply;
    co_return reply;
}

QCoro::Task<QNetworkReply *> WebDavClient::propfind(const QString &path, int depth,
                                                    const QList<WebDavPropertyRequest> &properties)
{
    auto request = createNetworkRequest(path);
    request.setRawHeader("Depth", QByteArray::number(depth));

    QNetworkReply *reply = co_await m_networkAccessManager.sendCustomRequest(
            request, "PROPFIND", m_propfindParser.buildPropfindBody(properties));
    co_return reply;
}

QCoro::Task<QNetworkReply *> WebDavClient::report(const QString &path, const QByteArray &body,
                                                  int depth)
{
    auto request = createNetworkRequest(path);
    request.setRawHeader("Depth", QByteArray::number(depth));
    QNetworkReply *reply =
            co_await m_networkAccessManager.sendCustomRequest(request, "REPORT", body);
    co_return reply;
}

QCoro::Task<WebDavSyncChanges> WebDavClient::syncCollection(const QString &path, QString syncToken)
{
    // Protect against truncated responses
    static constexpr uint maxSyncRounds = 100;

    WebDavSyncChanges changes;
    const auto collectionPath = WebDavItem::normalizedPath(m_connectionConfig.url.resolved(path));

    for (uint round = 0; round < maxSyncRounds; ++round) {

        // Make request for etag
        auto *reply = co_await report(path,
                                      m_propfindParser.buildSyncCollectionBody(
                                              syncToken, { { "D", "DAV:", "getetag" } }));
        const auto error = reply->error();
        const auto httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = reply->readAll();
        reply->deleteLater();

        // Handle errors
        if (error == QNetworkReply::AuthenticationRequiredError) {
            changes.result = WebDavSyncResult::AuthRequired;
            co_return changes;
        }
        if ((httpStatus == 403 || httpStatus == 409)
            && m_propfindParser.isInvalidSyncTokenError(body)) {
            changes.result = WebDavSyncResult::InvalidSyncToken;
            co_return changes;
        }
        if (error != QNetworkReply::NoError) {
            qCWarning(lcWebDavClient) << "sync-collection REPORT failed for" << collectionPath
                                      << "with HTTP status" << httpStatus << ":" << body.left(500);
            changes.result = WebDavSyncResult::OtherError;
            co_return changes;
        }

        // Handle good result
        const auto multiStatus = m_propfindParser.parseMultiStatusDocument(body);
        bool isTruncated = false;

        for (const auto &item : std::as_const(multiStatus.items)) {
            const auto itemPath = item.normalizedPath();

            if (itemPath == collectionPath) {
                isTruncated = item.httpStatus == 507;
                continue;
            }

            if (item.httpStatus == 404) {
                changes.changedHrefs.remove(itemPath);
                changes.removedPaths.insert(itemPath);
            } else if (item.httpStatus >= 200 && item.httpStatus < 300) {
                changes.removedPaths.remove(itemPath);
                changes.changedHrefs.insert(itemPath, item.href);
            }
        }

        if (!isTruncated) {
            co_return changes;
        }

        // Handle truncation
        if (multiStatus.syncToken.isEmpty() || multiStatus.syncToken == syncToken) {
            changes.result = WebDavSyncResult::OtherError;
            co_return changes;
        }

        syncToken = multiStatus.syncToken;
    }

    changes.result = WebDavSyncResult::OtherError;
    co_return changes;
}

void WebDavClient::onAuthenticationRequired(QNetworkReply *, QAuthenticator *authenticator)
{
    authenticator->setUser(m_connectionConfig.username);
    authenticator->setPassword(m_connectionConfig.password);
}
