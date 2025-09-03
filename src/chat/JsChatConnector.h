#pragma once

#include "JsConnectorConfig.h"
#include "IChatProvider.h"

#include <QObject>
#include <QQmlEngine>

class IChatRoom;
class JsChatEvent;
class JsChatUser;
class JsChatRoom;

class JsChatConnector : public IChatProvider
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by ChatConnectorManager")

    Q_PROPERTY(
            bool isSecretInitalized READ isSecretInitalized NOTIFY isSecretInitalizedChanged FINAL)
    Q_PROPERTY(QString ownUserId READ ownUserId CONSTANT FINAL)
    Q_PROPERTY(QString displayName READ displayName CONSTANT FINAL)
    Q_PROPERTY(QString deviceId READ deviceId CONSTANT FINAL)
    Q_PROPERTY(QString recoveryKey READ recoveryKey CONSTANT FINAL)
    Q_PROPERTY(QString accessToken READ accessToken NOTIFY accessTokenChanged FINAL)

public:
    explicit JsChatConnector(const JsConnectorConfig &config, QObject *parent = nullptr);

    QString ownUserId() const { return m_config.ownUserId; }
    QString deviceId() const { return m_config.deviceId; }
    QString displayName() const { return m_config.displayName; }
    QString recoveryKey() const { return m_config.recoveryKey; }
    QString accessToken() const { return m_config.accessToken; }

    bool isSecretInitalized() const { return m_isSecretInitalized; }

    /// Call this to handle a user-entered recovery key
    Q_INVOKABLE void handleRecoveryKey(const QString &key);

    /// To be called by JS part when token is known
    Q_INVOKABLE void handleAccessToken(const QString &token);

    /// To add an existing room via JS api; does not create a new chat room "on the server"
    Q_INVOKABLE void addChatRoom(const QString &roomId, const QString name);

    /// To add an existing message event via JS api; does not create a message "on the server"
    Q_INVOKABLE void addMessageEvent(const QString &eventId, const QString &roomId,
                                     const QString &senderId, const QString &message,
                                     const QDateTime &dateTime);

    /// To add an existing image event via JS api; does not create an image "on the server"
    Q_INVOKABLE void addImageEvent(const QString &eventId, const QString &roomId,
                                   const QString &senderId, const QString &imageUrl,
                                   const QDateTime &dateTime);

    /// To add an existing user via JS api; does not create a new user "on the server"
    Q_INVOKABLE void addUser(const QString &userId, const QString &displayName);
    JsChatUser *userById(const QString &userId) const { return m_users.value(userId, nullptr); }

    Q_INVOKABLE void updateRoomNotificationCount(const QString &roomId, quint16 count);

    virtual QList<IChatRoom *> chatRooms() override { return m_rooms; }
    virtual qsizetype indexOf(IChatRoom *chatRoom) const override;
    virtual IChatRoom *chatRoomByRoomId(const QString &roomId) const override;

    const QList<JsChatEvent *> &events() const { return m_events; }

private:
    void connect();
    void setIsSecretInitalized(bool value);
    JsChatRoom *createOrLookupChatRoom(const QString &roomId, const QString &roomName = "");

    JsConnectorConfig m_config;
    bool m_isSecretInitalized = false;
    QList<IChatRoom *> m_rooms;
    QHash<QString, JsChatRoom *> m_roomLookup;
    QList<JsChatEvent *> m_events;
    QHash<QString, JsChatUser *> m_users;

signals:
    void isSecretInitalizedChanged();
    void accessTokenChanged();
    void chatEventAdded();
    void chatUserAdded();
    void sendMessageRequested(QString roomId, QString message);
    void resetUnreadCountRequested(QString roomId);
};
