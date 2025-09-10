#pragma once
#include <QObject>
#include <QtQml/qqml.h>
#include <QtQml/qqmlregistration.h>
#include "Toggler.h"
#include "ReadOnlyConfdSettings.h"

class TogglerManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(TogglerManager)

public:
    Q_REQUIRED_RESULT static TogglerManager &instance()
    {
        static TogglerManager *_instance = nullptr;

        if (_instance == nullptr) {
            _instance = new TogglerManager();
        }

        return *_instance;
    }

    Toggler *getToggler(const QString &id);
    QList<Toggler *> toggler() const { return m_toggler; }

    Q_INVOKABLE void toggleToggler(const QString &id);

    ~TogglerManager() = default;

    void initialize();

Q_SIGNALS:
    void togglerChanged();
    void togglerActiveChanged(Toggler *toggler, bool value);
    void togglerBusyChanged(Toggler *toggler, bool value);

private:
    TogglerManager(QObject *parent = nullptr);

    QList<Toggler *> m_toggler;

    ReadOnlyConfdSettings m_settings;
};

class TogglerManagerWrapper
{
    Q_GADGET
    QML_FOREIGN(TogglerManager)
    QML_NAMED_ELEMENT(TogglerManager)
    QML_SINGLETON

public:
    static TogglerManager *create(QQmlEngine *, QJSEngine *) { return &TogglerManager::instance(); }

private:
    TogglerManagerWrapper() = default;
};
