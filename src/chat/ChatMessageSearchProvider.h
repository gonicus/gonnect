#pragma once

#include <QObject>
#include <QQmlEngine>

#include "IChatRoom.h"

#include "ChatMessageSearchModel.h"

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

    void resetChatProviders();

private:
    ChatMessageSearchProvider(QObject *parent = nullptr);

    ChatMessageSearchModel *m_model = nullptr;

    QString m_searchPhrase;

    QList<IChatProvider *> m_chatProviders;
    QObject *m_chatProviderContext = nullptr;

    QHash<QString, IChatRoom *> m_chatRoomsByUid;
    QHash<QString, QObject *> m_chatRoomContextsByUid;

Q_SIGNALS:
    void chatRoomAdded(QString uid);
    void chatRoomDeleted(QString uid);

    void chatMessagesChanged();

    void searchPhraseChanged();
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
        return &ChatMessageSearchProvider::instance();
    }

private:
    ChatMessageSearchProviderWrapper() = default;
};
