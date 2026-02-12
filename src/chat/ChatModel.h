#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>

#include "IChatRoom.h"

class ChatModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(IChatRoom *chatRoom MEMBER m_chatRoom NOTIFY chatRoomChanged FINAL)
    Q_PROPERTY(uint realMessagesCount READ realMessagesCount NOTIFY realMessagesCountChanged FINAL)

public:
    enum class Roles {
        EventId = Qt::UserRole + 1,
        FromId,
        Timestamp,
        NickName,
        Message,
        ImageUrl,
        IsPrivateMessage,
        IsOwnMessage,
        IsSystemMessage
    };
    explicit ChatModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    uint realMessagesCount() const { return m_realMessagesCount; }

private Q_SLOTS:
    void onChatRoomChanged();
    void updateRealMessagesCount();

private:
    QString addLinkTags(const QString &orig) const;

    IChatRoom *m_chatRoom = nullptr;
    QObject *m_chatRoomContext = nullptr;
    uint m_realMessagesCount = 0;

Q_SIGNALS:
    void chatRoomChanged();
    void realMessagesCountChanged();
};
