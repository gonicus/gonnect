#include "MatrixConnector.h"
#include "MatrixConnectorManager.h"
#include "MatrixMessageEvent.h"
#include "MatrixImageEvent.h"
#include "MatrixUser.h"
#include "ViewHelper.h"
#include "IChatRoom.h"
#include "ChatMessage.h"
#include "MatrixChatRoom.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcMatrixConnector, "gonnect.app.MatrixConnector")

MatrixConnector::MatrixConnector(const MatrixConnectorConfig &config, QObject *parent)
    : IChatProvider{ config.settingsGroup, parent }, m_config{ config }
{
    connect();
}

void MatrixConnector::connect()
{
    if (m_config.recoveryKey.isEmpty()) {
        auto &viewHelper = ViewHelper::instance();
        QObject::connect(&viewHelper, &ViewHelper::recoveryKeyResponded, this,
                         [this](const QString &id, const QString &key) {
                             if (id == m_config.settingsGroup) {
                                 ViewHelper::instance().disconnect(this);
                                 handleRecoveryKey(key);
                             }
                         });
        viewHelper.requestRecoveryKey(m_config.settingsGroup, m_config.displayName);
    } else {
        setIsSecretInitalized(true);
    }
}

qsizetype MatrixConnector::indexOf(IChatRoom *chatRoom) const
{
    return m_rooms.indexOf(chatRoom);
}

IChatRoom *MatrixConnector::chatRoomByRoomId(const QString &roomId) const
{
    return qobject_cast<IChatRoom *>(m_roomLookup.value(roomId, nullptr));
}

void MatrixConnector::handleRecoveryKey(const QString &key)
{
    MatrixConnectorManager::instance().saveRecoveryKey(m_config.settingsGroup, key);
    setIsSecretInitalized(true);
}

void MatrixConnector::handleAccessToken(const QString &value)
{
    MatrixConnectorManager::instance().saveAccessToken(m_config.settingsGroup, value);
    m_config.accessToken = value;
    emit accessTokenChanged();
}

void MatrixConnector::addMatrixRoom(const QString &roomId, const QString name)
{
    createOrLookupChatRoom(roomId, name);
}

void MatrixConnector::addMessageEvent(const QString &eventId, const QString &roomId,
                                      const QString &senderId, const QString &message,
                                      const QDateTime &dateTime)
{
    ChatMessage::Flags flags;
    if (senderId == matrixId()) {
        flags |= ChatMessage::Flag::OwnMessage;
    }

    auto room = createOrLookupChatRoom(roomId);
    room->addMessage(new MatrixMessageEvent(eventId, roomId, senderId, dateTime, message, flags));
}

void MatrixConnector::addImageEvent(const QString &eventId, const QString &roomId,
                                    const QString &senderId, const QString &imageUrl,
                                    const QDateTime &dateTime)
{
    m_events.append(new MatrixImageEvent(eventId, roomId, senderId, dateTime, imageUrl, this));
    emit matrixEventAdded();
}

void MatrixConnector::addUser(const QString &userId, const QString &displayName)
{
    auto user = new MatrixUser(userId, displayName, this);
    m_users.insert(userId, user);
    emit matrixUserAdded();
}

void MatrixConnector::updateRoomNotificationCount(const QString &roomId, quint16 count)
{
    auto room = m_roomLookup.value(roomId, nullptr);
    if (!room) {
        qCWarning(lcMatrixConnector) << "Unable to find room object for id" << roomId;
        return;
    }

    room->setUnreadNotificationCount(count);
}

void MatrixConnector::setIsSecretInitalized(bool value)
{
    if (m_isSecretInitalized != value) {
        m_isSecretInitalized = value;
        emit isSecretInitalizedChanged();
    }
}

MatrixChatRoom *MatrixConnector::createOrLookupChatRoom(const QString &roomId,
                                                        const QString &roomName)
{
    MatrixChatRoom *room = m_roomLookup.value(roomId, nullptr);

    if (!room) {
        room = new MatrixChatRoom(roomId, roomName, this);
        m_rooms.append(room);
        m_roomLookup.insert(roomId, room);
        emit chatRoomAdded(m_rooms.length() - 1, room);
    } else if (!roomName.isEmpty()) {
        room->setName(roomName);
        emit chatRoomNameChanged(m_rooms.indexOf(room), room, roomName);
    }

    return room;
}
