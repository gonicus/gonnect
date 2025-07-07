#pragma once

#include "MatrixEvent.h"

#include <QUrl>

class MatrixImageEvent : public MatrixEvent
{
    Q_OBJECT
public:
    explicit MatrixImageEvent(const QString &eventId, const QString &roomId,
                              const QString &senderId, const QDateTime &dateTime,
                              const QString &imageUrl, QObject *parent = nullptr);

    QUrl imageUrl() const { return m_imageUrl; }

private:
    QUrl m_imageUrl;
};
