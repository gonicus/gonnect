#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>

class IChatRoom;
class ChatMessage;

struct ThreadSummary
{
    QString threadId;
    ChatMessage *firstMessage = nullptr;
    ChatMessage *lastMessage = nullptr;
    qsizetype unreadCount = 0;
};

class ChatThreadModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(IChatRoom *chatRoom READ chatRoom WRITE setChatRoom NOTIFY chatRoomChanged FINAL)

public:
    enum class Roles { ThreadId = Qt::UserRole + 1, FirstMessage, LastMessage, UnreadCount };

    explicit ChatThreadModel(QObject *parent = nullptr);

    IChatRoom *chatRoom() const { return m_chatRoom; }

    virtual QHash<int, QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

private:
    void setChatRoom(IChatRoom *room);

    IChatRoom *m_chatRoom = nullptr;
    QObject *m_chatRoomContext = nullptr;
    QList<ThreadSummary> m_threadSummaries;

private Q_SLOTS:
    void onChatRoomChanged();
    void rebuildThreadSummaries();

Q_SIGNALS:
    void chatRoomChanged();
};
