#pragma once

#include "ChatModel.h"

#include <QSortFilterProxyModel>
#include <QQmlEngine>

class IChatRoom;
class ChatMessage;
class ChatUser;

class ChatProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "sourceModel")

public:
    enum class Roles {
        ReadUsers = static_cast<int>(ChatModel::Roles::LastRole),
        IsLatestOwnMessage
    };

    explicit ChatProxyModel(QObject *parent = nullptr);

    virtual QHash<int, QByteArray> roleNames() const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

protected:
    virtual bool lessThan(const QModelIndex &sourceLeft,
                          const QModelIndex &sourceRight) const override;

private:
    QObject *m_sourceModelContext = nullptr;
    QObject *m_chatRoomContext = nullptr;

    QList<ChatUser *> readUsersFor(const IChatRoom *chatRoom, const ChatMessage *message) const;
    bool isValidOwnMessage(const QModelIndex &index) const;
    ChatMessage *ownMessageAt(qsizetype proxyIndex) const;

private Q_SLOTS:
    void onSourceModelChanged();
    void onChatRoomChanged();
    void invalidateProxyRoles();
};
