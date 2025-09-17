#include "JsChatImageEvent.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QByteArray>
#include <QFile>

#include <cryptopp/cryptlib.h>
#include <cryptopp/aes.h>
#include <cryptopp/filters.h>
#include <cryptopp/osrng.h>
#include <cryptopp/hex.h>
#include <cryptopp/ccm.h>


JsChatImageEvent::JsChatImageEvent(const QString &eventId, const QString &roomId,
                                   const QString &senderId, const QString &filename,
                                   const QDateTime &dateTime,
                                   const QString &imageUrl, const QString &key,
                                   const QString &iv, const QByteArray &accessToken,
                                   QObject *parent)
    : JsChatEvent{ eventId, roomId, senderId, dateTime, parent },
    m_imageUrl(imageUrl),
    m_key(key),
    m_iv(iv),
    m_filename(filename)
{
    // download the image and decrypt it
    qInfo() << " ===> imageUrl: " << imageUrl << ", key: " << key << ", iv: " << iv << ", filename:" << filename;

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
        qInfo() << "===> downloaded (image) data starts with:" << data.left(100) << "...";

        QByteArray *recovered = decryptA256CTR(&data);

        writeToFile(recovered); // instead of writing it, we want to render the image in right place in its chat dialog
    }
    else
    {
        qInfo() << "Error:" << reply->errorString();
    }
}

QByteArray*
JsChatImageEvent::decryptA256CTR(QByteArray* data)
{
    QByteArray dec_key = urlsafeBase64Decode(m_key);
    QByteArray dec_iv = urlsafeBase64Decode(m_iv).left(16); // only the first 16-bytes matter
    qInfo() << " ===> size of iv:" << dec_iv.size() << ", data:" << dec_iv;
    qInfo() << " ===> size of key:" << dec_key.size() << ", data:" << dec_key;

    CryptoPP::CTR_Mode<CryptoPP::AES>::Decryption dec;
    dec.SetKeyWithIV(reinterpret_cast<CryptoPP::byte*>(dec_key.data()), dec_key.size(),
                     reinterpret_cast<CryptoPP::byte*>(dec_iv.data()));

    std::string rec;
    CryptoPP::StringSource ss(reinterpret_cast<const CryptoPP::byte*>(data->data()), data->size(), true,
        new CryptoPP::StreamTransformationFilter(dec, new CryptoPP::StringSink(rec))
    );

    QByteArray *recovered = new QByteArray(QByteArray::fromStdString(rec));

    qInfo() << "===> recovered (image) data starts with:" << recovered->left(100) << "...";

    return recovered;
}

// write recovered images to files (for testing) -- remember: file names are not unique!
void
JsChatImageEvent::writeToFile(QByteArray *data)
{
    QFile file("/tmp/" + m_filename);
    file.open(QIODevice::WriteOnly);
    file.write(*data);
    file.close();
}

QByteArray
urlsafeBase64Decode(QString data)
{
    // Convert urlsafe-base64 to the regular symbol set
    QByteArray ba = data.toUtf8();
    ba.replace('-', '+');
    ba.replace('_', '/');

    // Re-add the padding that matrix has stripped
    int padding = 4 - ba.size() % 4;
    if (padding < 4) {
        ba.append(QByteArray(padding, '='));
    }

    return QByteArray::fromBase64(ba);
}
