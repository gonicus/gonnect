#pragma once

#include "JsChatEvent.h"

#include <QUrl>
#include <QNetworkReply>

class JsChatImageEvent : public JsChatEvent
{
    Q_OBJECT
public:
    explicit JsChatImageEvent(const QString &eventId, const QString &roomId,
                              const QString &senderId, const QDateTime &dateTime,
                              const QString &imageUrl, const QString &key,
                              const QString &iv, const QByteArray &accessToken,
                              QObject *parent = nullptr);

    QUrl imageUrl() const { return m_imageUrl; }
    void downloadImage(QByteArray);

private:
    QUrl m_imageUrl;
    QByteArray m_imageData;
    QNetworkAccessManager m_manager;
    void onDownloadFinished(QNetworkReply*);
};
