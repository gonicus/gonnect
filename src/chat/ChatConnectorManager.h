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
    Q_PROPERTY(qsizetype unreadNotificationsCount READ unreadNotificationsCount NOTIFY
                       unreadNotificationsCountChanged FINAL)

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

    qsizetype unreadNotificationsCount() const { return m_unreadNotificationsCount; }

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
    void updateUnreadNotificationsCount();

    bool m_isInitialized = false;
    QHash<QString, SettingGroupStates *> m_groupStates;
    QList<IChatProvider *> m_chatProviders;

    qsizetype m_unreadNotificationsCount = 0;

Q_SIGNALS:
    void isChatAvailableChanged();
    void chatConnectorsChanged();
    void unreadNotificationsCountChanged();
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
        QQmlEngine::setObjectOwnership(&ChatConnectorManager::instance(), QQmlEngine::CppOwnership);
        return &ChatConnectorManager::instance();
    }

private:
    ChatConnectorManagerWrapper() = default;
};
