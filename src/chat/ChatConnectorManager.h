#pragma once

#include <QObject>
#include <QQmlEngine>

#include "IChatProvider.h" // Cannot be forward-declarated or the chatConnectors proerty will not work in qml
#include "IpcConfig.h"

class ChatConnectorManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isChatAvailable READ isChatAvailable NOTIFY isChatAvailableChanged FINAL)
    Q_PROPERTY(QList<IChatProvider *> chatConnectors READ chatConnectors NOTIFY
                       chatConnectorsChanged FINAL)

public:
    static ChatConnectorManager &instance()
    {
        static ChatConnectorManager *_instance = nullptr;
        if (!_instance) {
            _instance = new ChatConnectorManager;
        }
        return *_instance;
    }

    static QString hashForSettingsGroup(const QString &group);

    virtual ~ChatConnectorManager();

    bool isInitialized() const { return m_isInitialized; }
    bool isChatAvailable() const { return !m_chatProviders.isEmpty(); }
    QList<IChatProvider *> chatConnectors() const { return m_chatProviders; }

private Q_SLOTS:
    void init();

private:
    struct SettingGroupStates
    {
        ~SettingGroupStates();
        bool isLoginSecretRequired = false;
        bool isLoginSecretInitialized = false;
        bool isWaitingForLoginSecretResponse = false;
        bool isPersistentSecretInitialized = false;
        bool isWaitingForPersistentSecretResponse = false;
        bool isEncryptionSecretInitialized = false;
        bool isWaitingForEncryptionSecretResponse = false;
        IpcConfig *config = nullptr;
    };

    void saveSecret(const QString &settingsGroup, const QString &secret) const;
    explicit ChatConnectorManager(QObject *parent = nullptr);
    void processSettingGroup(const QString &group);

    bool m_isInitialized = false;
    QHash<QString, SettingGroupStates *> m_groupStates;
    QList<IChatProvider *> m_chatProviders;

Q_SIGNALS:
    void isChatAvailableChanged();
    void chatConnectorsChanged();
};

class ChatConnectorManagerWrapper
{
    Q_GADGET
    QML_FOREIGN(ChatConnectorManager)
    QML_NAMED_ELEMENT(ChatConnectorManager)
    QML_SINGLETON

public:
    static ChatConnectorManager *create(QQmlEngine *, QJSEngine *)
    {
        return &ChatConnectorManager::instance();
    }

private:
    ChatConnectorManagerWrapper() = default;
};
