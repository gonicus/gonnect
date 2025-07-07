#pragma once

#include "MatrixConnectorConfig.h"
#include "IChatProvider.h"

#include <QObject>
#include <QQmlEngine>

class IChatRoom;
class MatrixEvent;
class MatrixUser;
class MatrixChatRoom;

class MatrixConnector : public IChatProvider
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by MatrixConnectorManager")

    Q_PROPERTY(
            bool isSecretInitalized READ isSecretInitalized NOTIFY isSecretInitalizedChanged FINAL)
    Q_PROPERTY(QString matrixId READ matrixId CONSTANT FINAL)
    Q_PROPERTY(QString displayName READ displayName CONSTANT FINAL)
    Q_PROPERTY(QString deviceId READ deviceId CONSTANT FINAL)
    Q_PROPERTY(QString recoveryKey READ recoveryKey CONSTANT FINAL)
    Q_PROPERTY(QString accessToken READ accessToken NOTIFY accessTokenChanged FINAL)

public:
    explicit MatrixConnector(const MatrixConnectorConfig &config, QObject *parent = nullptr);

    QString matrixId() const { return m_config.matrixId; }
    QString deviceId() const { return m_config.deviceId; }
    QString displayName() const { return m_config.displayName; }
    QString recoveryKey() const { return m_config.recoveryKey; }
    QString accessToken() const { return m_config.accessToken; }

    bool isSecretInitalized() const { return m_isSecretInitalized; }

    /// Call this to handle a user-entered recovery key
    Q_INVOKABLE void handleRecoveryKey(const QString &key);

    /// To be called by JS part when token is known
    Q_INVOKABLE void handleAccessToken(const QString &token);

    /// To add an existing room via JS api; does not create a new matrix room "on the server"
    Q_INVOKABLE void addMatrixRoom(const QString &roomId, const QString name);

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
    MatrixUser *userById(const QString &userId) const { return m_users.value(userId, nullptr); }

    Q_INVOKABLE void updateRoomNotificationCount(const QString &roomId, quint16 count);

    virtual QList<IChatRoom *> chatRooms() override { return m_rooms; }
    virtual qsizetype indexOf(IChatRoom *chatRoom) const override;
    virtual IChatRoom *chatRoomByRoomId(const QString &roomId) const override;

    const QList<MatrixEvent *> &events() const { return m_events; }

private:
    void connect();
    void setIsSecretInitalized(bool value);
    MatrixChatRoom *createOrLookupChatRoom(const QString &roomId, const QString &roomName = "");

    MatrixConnectorConfig m_config;
    bool m_isSecretInitalized = false;
    QList<IChatRoom *> m_rooms;
    QHash<QString, MatrixChatRoom *> m_roomLookup;
    QList<MatrixEvent *> m_events;
    QHash<QString, MatrixUser *> m_users;

signals:
    void isSecretInitalizedChanged();
    void accessTokenChanged();
    void matrixEventAdded();
    void matrixUserAdded();
    void sendMessageRequested(QString roomId, QString message);
    void resetUnreadCountRequested(QString roomId, const QString &eventId);
};
