#include "WebDavClient.h"
#include "CertHelper.h"

#include <QAuthenticator>
#include <ReadOnlyConfdSettings.h>

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

void WebDavClient::onAuthenticationRequired(QNetworkReply *, QAuthenticator *authenticator)
{
    authenticator->setUser(m_connectionConfig.username);
    authenticator->setPassword(m_connectionConfig.password);
}
