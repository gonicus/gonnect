#pragma once

#include "IChatRoom.h"
#include "ChatMessageSearchIndexer.h"
#include "ChatMessageSearchModel.h"
#include "AppSettings.h"

#include <memory>

#include <QObject>
#include <QQmlEngine>
#include <QTimer>

class IChatProvider;

class ChatMessageSearchProvider : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ChatMessageSearchProvider)

    Q_PROPERTY(ChatMessageSearchModel *model READ model CONSTANT)
    Q_PROPERTY(QString searchPhrase MEMBER m_searchPhrase NOTIFY searchPhraseChanged)

public:
    Q_REQUIRED_RESULT static ChatMessageSearchProvider &instance()
    {
        static ChatMessageSearchProvider *_instance = nullptr;

        if (_instance == nullptr) {
            _instance = new ChatMessageSearchProvider();
        }

        return *_instance;
    }

    ~ChatMessageSearchProvider();

    ChatMessageSearchModel *model() const { return m_model; }

    Q_INVOKABLE IChatProvider *getChatProviderByRoom(const QString &roomUid);

    void updateChatProviders();
    void loadChatProvider(IChatProvider *provider);

    void loadChatRoom(const QString &providerUid, IChatRoom *room);
    void removeChatRoom(IChatRoom *room);

    void removeChatRoomsByProvider(const QString &providerUid);

    Q_INVOKABLE ChatMessage *getChatMessage(const QString &roomUid, const QString &messageUid);
    Q_INVOKABLE QString getChatMessageText(ChatMessage *message);

    Q_INVOKABLE bool isRoomIndexingEnabled(const QString &roomUid) const;
    Q_INVOKABLE void setRoomIndexingEnabled(const QString &roomUid, bool enabled);

    bool isRoomBackfillComplete(const QString &roomUid) const;
    void setRoomBackfillComplete(const QString &roomUid, bool isComplete);

private Q_SLOTS:
    void onBackfillTick();

private:
    ChatMessageSearchProvider(QObject *parent = nullptr);
    QString indexingSettingsKey(const QString &roomUid) const;
    QString backfillSettingsKey(const QString &roomUid) const;
    void indexRoomMessages(IChatRoom &room) const;

    ChatMessageSearchIndexer *m_indexer = nullptr;
    ChatMessageSearchModel *m_model = nullptr;
    AppSettings m_appSettings;
    QTimer m_backfillTimer;

    QString m_searchPhrase;

    struct ProviderConnection
    {
        std::shared_ptr<QObject> context;
        IChatProvider *provider = nullptr;

        ProviderConnection() : context(std::make_shared<QObject>()) { }

        ProviderConnection(IChatProvider *provider)
            : context(std::make_shared<QObject>()), provider(provider)
        {
        }
    };

    struct RoomConnection
    {
        std::shared_ptr<QObject> context;
        QString providerUid;
        IChatRoom *room = nullptr;

        RoomConnection() : context(std::make_shared<QObject>()) { }

        RoomConnection(QString providerUid, IChatRoom *room)
            : context(std::make_shared<QObject>()), providerUid(providerUid), room(room)
        {
        }
    };

    QHash<QString, ProviderConnection> m_chatProvidersByUid;
    QHash<QString, RoomConnection> m_chatRoomsByUid;

Q_SIGNALS:
    void chatRoomAdded(QString uid);
    void chatRoomDeleted(QString uid);
    void chatMessagesChanged();
    void searchPhraseChanged();
    void roomIndexingEnabledChanged(QString roomUid, bool enabled);
};

class ChatMessageSearchProviderWrapper
{
    Q_GADGET
    QML_FOREIGN(ChatMessageSearchProvider)
    QML_NAMED_ELEMENT(ChatMessageSearchProvider)
    QML_SINGLETON

public:
    static ChatMessageSearchProvider *create(QQmlEngine *, QJSEngine *)
    {
        QQmlEngine::setObjectOwnership(&ChatMessageSearchProvider::instance(),
                                       QQmlEngine::CppOwnership);
        return &ChatMessageSearchProvider::instance();
    }

private:
    ChatMessageSearchProviderWrapper() = default;
};
