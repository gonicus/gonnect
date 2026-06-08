#pragma once

#include <QObject>
#include <QQmlEngine>

class ClipboardHelper : public QObject
{
    Q_OBJECT

public:
    static ClipboardHelper &instance()
    {
        static ClipboardHelper *_instance = nullptr;
        if (!_instance) {
            _instance = new ClipboardHelper;
        }
        return *_instance;
    };

    Q_INVOKABLE void copyToClipboard(const QString &str) const;
    Q_INVOKABLE void copyImageToClipboard(const QString &imagePath) const;
    Q_INVOKABLE bool hasText() const;
    Q_INVOKABLE bool hasImage() const;

    QImage image() const;

private:
    explicit ClipboardHelper(QObject *parent = nullptr);
};

class ClipboardHelperWrapper
{
    Q_GADGET
    QML_FOREIGN(ClipboardHelper)
    QML_NAMED_ELEMENT(ClipboardHelper)
    QML_SINGLETON

public:
    static ClipboardHelper *create(QQmlEngine *, QJSEngine *)
    {
        return &ClipboardHelper::instance();
    }

private:
    ClipboardHelperWrapper() = default;
};
