#pragma once

#include <QObject>
#include <QQmlEngine>

class TextFormatHelper : public QObject
{
    Q_OBJECT

public:
    static TextFormatHelper &instance()
    {
        static TextFormatHelper *_instance = nullptr;
        if (!_instance) {
            _instance = new TextFormatHelper;
        }
        return *_instance;
    }

    Q_INVOKABLE QString formatFileSize(qint64 byteSize) const;

private:
    explicit TextFormatHelper(QObject *parent = nullptr);
};

class TextFormatHelperWrapper
{
    Q_GADGET
    QML_FOREIGN(TextFormatHelper)
    QML_NAMED_ELEMENT(TextFormatHelper)
    QML_SINGLETON

public:
    static TextFormatHelper *create(QQmlEngine *, QJSEngine *)
    {
        return &TextFormatHelper::instance();
    }

private:
    TextFormatHelperWrapper() = default;
};
