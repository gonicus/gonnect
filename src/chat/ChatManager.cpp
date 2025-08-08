#include "ChatManager.h"
#include "IChatProvider.h"
#include "ReadOnlyConfdSettings.h"
#include "MatrixChatProvider.h"

#include <QLoggingCategory>
#include <QRegularExpression>

Q_LOGGING_CATEGORY(lcChatManager, "gonnect.app.chat.ChatManager")

ChatManager::ChatManager(QObject *parent) : QObject{ parent }
{
    createProvidersFromConfig();
}

void ChatManager::createProvidersFromConfig()
{
    static QRegularExpression matrixGroupRegex = QRegularExpression("^matrix[0-9]+$");

    ReadOnlyConfdSettings settings;
    const QStringList groups = settings.childGroups();

    for (const auto &group : groups) {
        if (matrixGroupRegex.match(group).hasMatch()) {
            auto provider = new MatrixChatProvider(group, this);
            addChatProvider(provider);
        }
    }
}

void ChatManager::addChatProvider(IChatProvider *provider)
{
    Q_CHECK_PTR(provider);

    if (!m_chatProviders.contains(provider)) {
        provider->setParent(this);
        m_chatProviders.append(provider);

        QObject::connect(provider, &IChatProvider::isConnectedChanged, this,
                         &ChatManager::updateConnectionState);

        updateConnectionState();
    }
}

void ChatManager::connect()
{
    for (auto provider : std::as_const(m_chatProviders)) {
        if (!provider->isConnected()) {
            provider->connect();
        }
    }
}

IChatProvider *ChatManager::firstProvider() const
{
    if (m_chatProviders.size()) {
        return m_chatProviders.first();
    }
    return nullptr;
}

void ChatManager::updateConnectionState()
{
    qsizetype count = 0;
    ConnectionState newState = ConnectionState::Disconnected;

    for (const auto provider : std::as_const(m_chatProviders)) {
        if (provider->isConnected()) {
            ++count;
        }
    }

    if (count > 0) {
        if (count == m_chatProviders.size()) {
            newState = ConnectionState::AllConnected;
        } else {
            newState = ConnectionState::PartiallyConnected;
        }
    }

    if (m_connectionState != newState) {
        m_connectionState = newState;
        emit connectionStateChanged();

        qCInfo(lcChatManager) << "Chat manager connection state changed to" << newState << "with"
                              << count << "providers connected";
    }
}
