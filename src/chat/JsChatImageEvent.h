#pragma once

#include "JsChatEvent.h"

#include <QUrl>
#include <QNetworkReply>

class JsChatImageEvent : public JsChatEvent
{
    Q_OBJECT
public:
    explicit JsChatImageEvent(const QString &eventId, const QString &roomId,
                              const QString &senderId, const QString &filename,
                              const QDateTime &dateTime,
                              const QString &imageUrl, const QString &key,
                              const QString &iv, const QByteArray &accessToken,
                              QObject *parent = nullptr);

    QUrl imageUrl() const { return m_imageUrl; }
    void downloadImage(QByteArray);

private:
    QUrl m_imageUrl;
    QByteArray m_imageData;
    QString m_key, m_iv, m_filename;
    QNetworkAccessManager m_manager;

    void onDownloadFinished(QNetworkReply*);
    void writeToFile(QByteArray *data);
    QByteArray* decryptA256CTR(QByteArray* data);
};

QByteArray urlsafeBase64Decode(QString data);
