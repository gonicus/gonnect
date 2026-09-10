#pragma once

#include <QAbstractListModel>
#include <qqmlintegration.h>

#include "IChatProvider.h"

class IChatRoom;

class ChatRoomModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(IChatProvider *chatProvider MEMBER m_chatProvider NOTIFY chatProviderChanged FINAL)

public:
    ChatRoomModel(QObject *parent = nullptr);

    enum class Roles {
        RoomId = Qt::UserRole + 1,
        Name,
        AvatarPath,
        UnreadCount,
        IsFavorite,
        HasPresenceState,
        PresenceState,
        JoinRule,
        LatestMessageDate,
        ChatProvider,
        Permissions,
        OwnJoinState,
        TypingUserNames,

        // Dummy element such that the enum can be extended in ChatRoomProxyModel; must remain the
        // last/highest value in this enum
        LastRole
    };

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QModelIndex indexForRoomId(const QString &roomId) const;

private:
    void connectChatRoomSignals(IChatRoom *chatRoom);
    void emitDataChanged(IChatRoom *chatRoom, const QList<int> &roles);

    IChatProvider *m_chatProvider = nullptr;
    QObject *m_chatProviderContext = nullptr;

    QHash<IChatRoom *, QObject *> m_chatRoomContextObjects;

private Q_SLOTS:
    void onChatProviderChanged();

Q_SIGNALS:
    void chatProviderChanged();
};
