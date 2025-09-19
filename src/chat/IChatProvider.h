#pragma once

#include <QObject>

class IChatRoom;

class IChatProvider : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY isConnectedChanged FINAL)

public:
    explicit IChatProvider(const QString &settingsGroup, QObject *parent = nullptr);
    virtual ~IChatProvider() { }

    bool isConnected() const { return m_isConnected; }

    virtual QList<IChatRoom *> chatRooms() = 0;
    virtual qsizetype indexOf(IChatRoom *chatRoom) const = 0;
    Q_INVOKABLE virtual IChatRoom *chatRoomByRoomId(const QString &roomId) const = 0;
    virtual void reactToMessage(IChatRoom *chatRoom, const QString &eventId,
                                const QString &emoji) = 0;

protected:
    void setIsConnected(bool value);

    bool m_isConnected = false;
    QString m_settingsGroup;

Q_SIGNALS:
    void isConnectedChanged();

    void chatRoomAdded(qsizetype index, IChatRoom *);
    void chatRoomNameChanged(qsizetype index, IChatRoom *, QString name);
    void chatRoomNotificationCountChanged(qsizetype index, IChatRoom *, qsizetype count);
    void chatRoomRemoved(qsizetype index, IChatRoom *);
};
