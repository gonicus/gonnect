#pragma once

#include <QObject>
#include <QQmlEngine>

class ErrorBus : public QObject
{
    Q_OBJECT

public:
    static ErrorBus &instance()
    {
        static ErrorBus *_instance = nullptr;
        if (!_instance) {
            _instance = new ErrorBus;
        }
        return *_instance;
    }

    void addError(const QString &message);
    void addFatalError(const QString &message);

private:
    explicit ErrorBus(QObject *parent = nullptr);

signals:
    void error(QString message);
    void fatalError(QString message);
};

class ErrorBusWrapper
{
    Q_GADGET
    QML_FOREIGN(ErrorBus)
    QML_NAMED_ELEMENT(ErrorBus)
    QML_SINGLETON

public:
    static ErrorBus *create(QQmlEngine *, QJSEngine *) { return &ErrorBus::instance(); }

private:
    ErrorBusWrapper() = default;
};
