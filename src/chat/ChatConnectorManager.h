#pragma once

#include <QObject>
#include <QQmlEngine>

class ChatConnectorManager : public QObject
{
    Q_OBJECT

public:
    static ChatConnectorManager &instance()
    {
        static ChatConnectorManager *_instance = nullptr;
        if (!_instance) {
            _instance = new ChatConnectorManager;
        }
        return *_instance;
    }

    bool isInitialized() const { return m_isInitialized; }

    void saveRecoveryKey(const QString &settingsGroup, const QString &key) const;
    void saveAccessToken(const QString &settingsGroup, const QString &token) const;

private Q_SLOTS:
    void init();

private:
    explicit ChatConnectorManager(QObject *parent = nullptr);

    bool m_isInitialized = false;
    QHash<QString, quint8> m_waitingCallbackCount;
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
