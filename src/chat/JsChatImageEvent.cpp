#include "JsChatImageEvent.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

JsChatImageEvent::JsChatImageEvent(const QString &eventId, const QString &roomId,
                                   const QString &senderId, const QDateTime &dateTime,
                                   const QString &imageUrl, const QString &key,
                                   const QString &iv, const QByteArray &accessToken,
                                   QObject *parent)
    : JsChatEvent{ eventId, roomId, senderId, dateTime, parent }, m_imageUrl(imageUrl)
{
    // qCritical() << "===> 1" << imageUrl << "2" << m_imageUrl;
    // download the image and decrypt it
    qInfo() << " ===> imageUrl: " << imageUrl << ", key: " << key << ", iv: " << iv;
    downloadImage(accessToken);
}

void
JsChatImageEvent::downloadImage(QByteArray accessToken)
{
    qInfo() << "===> downloading the image using access token " << accessToken;

    connect(&m_manager, &QNetworkAccessManager::finished,
                this, &JsChatImageEvent::onDownloadFinished);

    QNetworkRequest request(m_imageUrl);
    request.setRawHeader("Authorization", "Bearer " + accessToken);
    m_manager.get(request);
}

void
JsChatImageEvent::onDownloadFinished(QNetworkReply *reply)
{
    qInfo() << "===> image download finished";
    if (reply->error() == QNetworkReply::NoError)
    {
        QByteArray data = reply->readAll();
        qInfo() << "Downloaded (image) data starts with:" << data.left(100) << "...";
    }
    else
    {
        qInfo() << "Error:" << reply->errorString();
    }
}
