#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QDateTime>
#include <QQmlEngine>
#include <QSet>

class CallHistoryItem;
class ChatMessage;
class Contact;
class IChatRoom;
class IChatProvider;

class ActivitiesModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int limit MEMBER m_limit NOTIFY limitChanged FINAL)

public:
    enum class Kind { SIPCall, JitsiMeetCall, ChatMessage };

    enum class Roles {
        Id = Qt::UserRole + 1,
        Day,
        Time,
        Kind,
        IsSIPCall,
        IsJitsiMeetCall,
        IsChatMessage,
        Title,
        Subtitle,
        Text,
        Location,
        HasAvatar,
        AvatarPath,
        Account,
        ContactId,
        RemoteUrl,
        RemotePhoneNumber,
        DurationSeconds,
        WasEstablished,
        IsAnonymous,
        IsFavorite,
        IsBlocked,
        HasBuddyState,
        Hops,
        CallType,
        ChatProviderId,
        ChatRoomId,
        IsOwnMessage,
    };

    explicit ActivitiesModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void removeEntry(qint64 id) const;

private:
    struct Entry
    {
        Kind kind = Kind::SIPCall;
        QDateTime time;

        // Valid for entries of the kinds SIPCall and JitsiMeetCall. The object is owned by the
        // CallHistory singleton and stays alive as long as the entry is used.
        CallHistoryItem *callItem = nullptr;

        // Valid for entries of the kind ChatMessage. The values are snapshotted so that the
        // entries survive a re-creation of their room object (e.g. on a reconnect).
        QString chatProviderId;
        QString chatRoomId;
        QString roomName;
        QString senderName;
        QString chatMessageId;
        QString messageText;
        QString avatarPath;
        bool isOwnMessage = false;
    };

    void populateCallHistory();
    void subscribeToProviders();
    void subscribeToRoom(IChatRoom *room);
    void handleCallItemAdded(qsizetype index, CallHistoryItem *item);
    void handleCallItemChanged(qsizetype index, CallHistoryItem *item);
    void handleCallItemRemoved(qsizetype index, CallHistoryItem *item);
    void handleChatMessageAdded(ChatMessage *message);
    void handleChatMessageContentChanged(qsizetype index, ChatMessage *item);
    void handleChatMessageRemoved(qsizetype index, ChatMessage *item);
    void insertEntry(Entry entry);
    void trimToLimit();
    void refreshCallRoles(const QList<int> &roles);

    QList<Entry> m_entries;
    QSet<Contact *> m_avatarTrackedContacts;
    QSet<IChatRoom *> m_subscribedRooms;
    QSet<IChatProvider *> m_subscribedProviders;
    int m_limit = -1;

Q_SIGNALS:
    void limitChanged();
};
