#pragma once

#include <QObject>
#include <QQmlEngine>

class WebEngineKeyEventFilter : public QObject
{
    Q_OBJECT

public:
    static WebEngineKeyEventFilter &instance()
    {
        static WebEngineKeyEventFilter _instance;
        return _instance;
    }

private:
    explicit WebEngineKeyEventFilter(QObject *parent = nullptr);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

Q_SIGNALS:
    void f11Pressed();
    void escapePressed();
    void ctrlFPressed();
    void ctrlKPressed();
    void ctrlShiftMPressed();
};

class WebengineKeyEventFilterWrapper
{
    Q_GADGET
    QML_FOREIGN(WebEngineKeyEventFilter)
    QML_NAMED_ELEMENT(WebEngineKeyEventFilter)
    QML_SINGLETON

public:
    static WebEngineKeyEventFilter *create(QQmlEngine *, QJSEngine *)
    {
        QQmlEngine::setObjectOwnership(&WebEngineKeyEventFilter::instance(),
                                       QQmlEngine::CppOwnership);
        return &WebEngineKeyEventFilter::instance();
    }

private:
    WebengineKeyEventFilterWrapper() = default;
};
