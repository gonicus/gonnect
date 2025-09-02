#pragma once

#include "JsChatEvent.h"

#include <QUrl>

class JsChatImageEvent : public JsChatEvent
{
    Q_OBJECT
public:
    explicit JsChatImageEvent(const QString &eventId, const QString &roomId,
                              const QString &senderId, const QDateTime &dateTime,
                              const QString &imageUrl, QObject *parent = nullptr);

    QUrl imageUrl() const { return m_imageUrl; }

private:
    QUrl m_imageUrl;
};
