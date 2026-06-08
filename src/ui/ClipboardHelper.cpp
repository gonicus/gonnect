#include "ClipboardHelper.h"

#include <QApplication>
#include <QClipboard>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcClipboardHelper, "gonnect.app.ui.ClipboardHelper")

ClipboardHelper::ClipboardHelper(QObject *parent) : QObject{ parent } { }

void ClipboardHelper::copyToClipboard(const QString &str) const
{
    auto clipboard = QApplication::clipboard();
    clipboard->setText(str, QClipboard::Clipboard);
    clipboard->setText(str, QClipboard::Selection);
}

void ClipboardHelper::copyImageToClipboard(const QString &imagePath) const
{
    const auto path = imagePath.startsWith("file://") ? imagePath.mid(7) : imagePath;

    const QImage img(path);
    if (img.isNull()) {
        qCCritical(lcClipboardHelper)
                << "Unable to copy image to clipboard because QImage for path" << path << "is null";
        return;
    }

    auto clipboard = QApplication::clipboard();
    clipboard->setImage(img, QClipboard::Clipboard);
    clipboard->setImage(img, QClipboard::Selection);
}

bool ClipboardHelper::hasText() const
{
    auto clipboard = QApplication::clipboard();
    return !clipboard->text().isEmpty();
}

bool ClipboardHelper::hasImage() const
{
    auto clipboard = QApplication::clipboard();
    return !clipboard->image().isNull();
}

QImage ClipboardHelper::image() const
{
    auto clipboard = QApplication::clipboard();
    return clipboard->image();
}
