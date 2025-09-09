#include <QBuffer>
#include <QImage>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QLoggingCategory>
#include "NotificationIcon.h"

Q_LOGGING_CATEGORY(lcNotifications, "gonnect.notifications")

NotificationIcon::NotificationIcon(const QString &fileUri, const QString &emblem, bool rounded)
{
    QImage image(fileUri);
    QImage tmpImage;

    if (image.isNull()) {
        m_imageName = fileUri;
        return;
    }

    // Add round shape to image?
    if (rounded) {
        tmpImage = QImage(image.size(), QImage::Format_ARGB32);
        tmpImage.fill(Qt::transparent);

        QPainter painter(&tmpImage);
        painter.setRenderHint(QPainter::Antialiasing, true);

        QPainterPath clipPath;
        clipPath.addEllipse(image.rect());

        painter.setClipPath(clipPath);
        painter.drawImage(QPoint(), image);
        painter.end();

        tmpImage = tmpImage.scaled(128, 128, Qt::IgnoreAspectRatio);
    } else {
        tmpImage = image.scaled(128, 128, Qt::IgnoreAspectRatio);
    }

    if (!emblem.isEmpty()) {
        QImage emblemImage(":/emblems/" + emblem + ".png");
        if (!emblemImage.isNull() && emblemImage.width() < 42 && emblemImage.height() < 42) {
            QPainter emblemPainter(&tmpImage);
            emblemPainter.drawImage(QPoint(128 - emblemImage.width(), 128 - emblemImage.height()),
                                    emblemImage);
            emblemPainter.end();
        } else {
            qCWarning(lcNotifications) << "unable to find emblem image" << emblem;
        }
    }

    QPixmap pixmap = QPixmap::fromImage(tmpImage);
    QBuffer buffer(&m_data);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG");
}
