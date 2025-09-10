#pragma once

#include <QObject>
#include <QtQml/qqml.h>
#include <QtQml/qqmlregistration.h>
#include "InhibitHelper.h"
#include "ICallState.h"

class DBusActivationAdapter;
class GOnnectDBusAPI;

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

#ifdef Q_OS_LINUX
    GOnnectDBusAPI *apiEndpoint() { return m_apiEndpoint; }
#endif

    void sendArguments(const QStringList &args);
    Q_INVOKABLE void restart();

    void inhibitScreenSaver();
    void releaseScreenSaver();

    ~StateManager();

    bool isFirstInstance() const { return m_isFirstInstance; }

Q_SIGNALS:
    void globalShortcutsSupportedChanged();
    void globalShortcutsChanged();

public Q_SLOTS:
    void Activate(const QVariantMap &platform_data);
    void ActivateAction(const QString &action_name, const QVariantList &parameter,
                        const QVariantMap &platform_data);
    void Open(const QStringList &uris, const QVariantMap &platform_data);

private:
    explicit StateManager(QObject *parent = nullptr);

    void sessionStateChanged(bool screensaverActive, InhibitHelper::InhibitState state);
    void updateInhibitState();

    InhibitHelper *m_inhibitHelper = nullptr;

    DBusActivationAdapter *m_activationAdapter = nullptr;
    GOnnectDBusAPI *m_apiEndpoint = nullptr;

    ICallState::States m_oldCallState = ICallState::State::Idle;

    bool m_isFirstInstance = true;
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
