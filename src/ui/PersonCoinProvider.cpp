#include "PersonCoinProvider.h"
#include "Theme.h"
#include <QDir>
#include <QPainter>
#include <QStandardPaths>

PersonCoinProvider::PersonCoinProvider() : QQuickImageProvider{ QQuickImageProvider::Image } { }

QImage PersonCoinProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    if (id.isEmpty()) {
        size->setWidth(0);
        size->setHeight(0);
        return QImage();
    }

    // Determine size
    int w = 100, h = 100;

    if (requestedSize.isValid() && requestedSize.width() > 0 && requestedSize.height() > 0) {
        w = requestedSize.width();
        h = requestedSize.height();

        if (w != h) {
            const int minValue = std::min(w, h);
            w = minValue;
            h = minValue;
        }
    }

    size->setWidth(w);
    size->setHeight(h);

    const auto path = makePath(id, w);
    if (QFileInfo::exists(path)) {
        return QImage(path);

    } else {

        // Draw image
        QImage image(*size, QImage::Format_ARGB32);
        image.fill(Qt::transparent);

        QPainter p(&image);
        Theme &theme = Theme::instance();

        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(theme.backgroundInitials());
        p.setPen(Qt::NoPen);
        p.drawEllipse(0, 0, w, h);

        // Initials
        Q_UNUSED(id);
        QFont font("Noto Sans");
        font.setPixelSize(0.4 * h);
        p.setFont(font);
        p.setPen(theme.foregroundInitials());
        p.drawText(image.rect(), Qt::AlignCenter, id);

        QFileInfo info(path);
        QDir dir;
        dir.mkpath(info.path());
        p.end();
        image.save(path);

        return image;
    }
}

QString PersonCoinProvider::makePath(const QString &id, const int size) const
{
    static QString basePath;
    if (basePath.isEmpty()) {
        basePath = QString("%1/coin").arg(
                QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
        QDir dir;
        dir.mkpath(basePath);
    }

    return QString("%1/%2/%3.png").arg(basePath).arg(size).arg(id);
}
