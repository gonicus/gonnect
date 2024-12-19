#pragma once

#include <QObject>
#include <QtQml/qqml.h>
#include <QtQml/qqmlregistration.h>
#include "InhibitPortal.h"

class DBusActivationAdapter;
class GlobalShortcutPortal;
class OrgFreedesktopScreenSaverInterface;

class StateManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool globalShortcutsSupported READ globalShortcutsSupported NOTIFY
                       globalShortcutsSupportedChanged FINAL)
    Q_PROPERTY(QVariantMap globalShortcuts READ globalShortcuts NOTIFY globalShortcutsChanged FINAL)

public:
    Q_REQUIRED_RESULT static StateManager &instance()
    {
        static StateManager *_instance = nullptr;

        if (_instance == nullptr) {
            _instance = new StateManager();
        }

        return *_instance;
    }

    void initialize();
    bool globalShortcutsSupported() const;
    QVariantMap globalShortcuts() const;

    void sendArguments(const QStringList &args);
    Q_INVOKABLE void restart();

    void inhibitScreenSaver();
    void releaseScreenSaver();

    ~StateManager();

    bool isFirstInstance() const { return m_isFirstInstance; }

signals:
    void globalShortcutsSupportedChanged();
    void globalShortcutsChanged();

public slots:
    void Activate(const QVariantMap &platform_data);
    void ActivateAction(const QString &action_name, const QVariantList &parameter,
                        const QVariantMap &platform_data);
    void Open(const QStringList &uris, const QVariantMap &platform_data);

private:
    explicit StateManager(QObject *parent = nullptr);

    void sessionStateChanged(bool screensaverActive, InhibitPortal::InhibitState state);

    GlobalShortcutPortal *m_globalShortcutPortal = nullptr;
    InhibitPortal *m_inhibitPortal = nullptr;
    OrgFreedesktopScreenSaverInterface *m_screenSaverInterface = nullptr;

    DBusActivationAdapter *m_activationAdapter = nullptr;

    unsigned m_screenSaverCookie = 0;

    bool m_isFirstInstance = true;
    bool m_screenSaverIsInhibited = false;
};

class StateManagerWrapper
{
    Q_GADGET
    QML_FOREIGN(StateManager)
    QML_NAMED_ELEMENT(SM)
    QML_SINGLETON

public:
    static StateManager *create(QQmlEngine *, QJSEngine *) { return &StateManager::instance(); }

private:
    StateManagerWrapper() = default;
};
