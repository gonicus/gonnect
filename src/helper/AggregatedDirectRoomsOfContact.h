#pragma once

#include <QObject>
#include <QQmlEngine>
#include "Contact.h"
#include "IChatRoom.h"

class AggregatedDirectRoomsOfContact : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(Contact *contact MEMBER m_contact NOTIFY contactChanged FINAL)
    Q_PROPERTY(QList<IChatRoom *> chatRooms READ chatRooms NOTIFY chatRoomsChanged FINAL)
    Q_PROPERTY(IChatRoom *bestMatchingChatRoom READ bestMatchingChatRoom NOTIFY
                       bestMatchingChatRoomChanged FINAL)

public:
    explicit AggregatedDirectRoomsOfContact(QObject *parent = nullptr);

    QList<IChatRoom *> chatRooms() const { return m_chatRooms; }
    IChatRoom *bestMatchingChatRoom() const { return m_bestRoom; }

    Q_INVOKABLE IChatProvider *providerOfRoom(IChatRoom *chatRoom) const;

private:
    void setChatRooms(const QList<IChatRoom *> chatRooms);
    void processProvider(IChatProvider *provider, QList<IChatRoom *> &chatRooms);
    bool processRoom(IChatRoom *room, QList<IChatRoom *> &chatRooms);
    void addRoom(IChatRoom *room);
    void removeRoom(IChatRoom *room);
    void sortChatRoomList(QList<IChatRoom *> &chatRooms) const;

    Contact *m_contact = nullptr;
    IChatRoom *m_bestRoom = nullptr;
    QList<IChatRoom *> m_chatRooms;
    QHash<IChatProvider *, QObject *> m_providerContextObjects;
    QHash<IChatRoom *, QObject *> m_roomContextObjects;

private Q_SLOTS:
    void onContactChanged();
    void updateBestMatchingRoom();

Q_SIGNALS:
    void contactChanged();
    void chatRoomsChanged();
    void bestMatchingChatRoomChanged();
};
