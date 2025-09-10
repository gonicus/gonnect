#pragma once

#include <QObject>
#include <QQmlEngine>

#include "JsChatConnector.h"

class ChatConnectorManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isJsChatAvailable READ isJsChatAvailable NOTIFY jsChatConnectorsChanged FINAL)
    Q_PROPERTY(QList<JsChatConnector *> jsChatConnectors READ jsChatConnectors NOTIFY
                       jsChatConnectorsChanged FINAL)

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
    bool isJsChatAvailable() const { return !m_connectors.isEmpty(); }
    QList<JsChatConnector *> jsChatConnectors() const { return m_connectors; };

    void saveRecoveryKey(const QString &settingsGroup, const QString &key) const;
    void saveAccessToken(const QString &settingsGroup, const QString &token) const;

private Q_SLOTS:
    void init();

private:
    explicit ChatConnectorManager(QObject *parent = nullptr);

    bool m_isInitialized = false;
    bool m_isJsChatAvailable = false;
    QList<JsChatConnector *> m_connectors;

Q_SIGNALS:
    void jsChatConnectorsChanged();
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
