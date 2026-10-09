#include "ChatMessageContentImage.h"

ChatMessageContentImage::ChatMessageContentImage(const QUrl &imagePath, QObject *parent)
    : QObject{ parent }, m_imagePath{ imagePath }
{
}

void ChatMessageContentImage::setImagePath(const QUrl &imagePath)
{
    if (m_imagePath != imagePath) {
        m_imagePath = imagePath;
        Q_EMIT imagePathChanged();
    }
}
