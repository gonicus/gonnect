#pragma once

#include <QObject>
#include <QQmlEngine>

class GlobalInfo : public QObject
{
    Q_OBJECT

public:
    static GlobalInfo &instance()
    {
        static GlobalInfo *_instance = nullptr;
        if (!_instance) {
            _instance = new GlobalInfo;
        }
        return *_instance;
    }

    Q_INVOKABLE QString jitsiUrl();

private:
    explicit GlobalInfo(QObject *parent = nullptr);

    bool m_isJitsiUrlInitialized = false;
    QString m_jitsiUrl;
};

class GlobalInfoWrapper
{
    Q_GADGET
    QML_FOREIGN(GlobalInfo)
    QML_NAMED_ELEMENT(GlobalInfo)
    QML_SINGLETON

public:
    static GlobalInfo *create(QQmlEngine *, QJSEngine *) { return &GlobalInfo::instance(); }

private:
    GlobalInfoWrapper() = default;
};
