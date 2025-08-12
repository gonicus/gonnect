#pragma once

#include <QObject>
#include <QQmlEngine>

class IChatProvider;

class ChatManager : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

    Q_PROPERTY(ChatManager::ConnectionState connectionState READ connectionState NOTIFY
                       connectionStateChanged FINAL)

public:
    enum class ConnectionState { Disconnected, PartiallyConnected, AllConnected };
    Q_ENUM(ConnectionState)

    static ChatManager &instance()
    {
        static ChatManager *_instance = nullptr;
        if (!_instance) {
            _instance = new ChatManager;
        }
        return *_instance;
    }

    ConnectionState connectionState() const { return m_connectionState; }

    void addChatProvider(IChatProvider *provider);
    void connect();

    IChatProvider *firstProvider() const;

private slots:
    void updateConnectionState();

private:
    explicit ChatManager(QObject *parent = nullptr);
    void createProvidersFromConfig();

    QList<IChatProvider *> m_chatProviders;
    ConnectionState m_connectionState = ConnectionState::Disconnected;

signals:
    void connectionStateChanged();
};

class ChatManagerWrapper
{
    Q_GADGET
    QML_FOREIGN(ChatManager)
    QML_NAMED_ELEMENT(ChatManager)
    QML_SINGLETON

public:
    static ChatManager *create(QQmlEngine *, QJSEngine *) { return &ChatManager::instance(); }

private:
    ChatManagerWrapper() = default;
};
