#include "JsChatConnector.h"
#include "ChatConnectorManager.h"
#include "JsChatMessageEvent.h"
#include "JsChatImageEvent.h"
#include "JsChatUser.h"
#include "ViewHelper.h"
#include "IChatRoom.h"
#include "ChatMessage.h"
#include "JsChatRoom.h"

#include <QLoggingCategory>
#include <QHttpServer>
#include <QTcpServer>

Q_LOGGING_CATEGORY(lcJsChatConnector, "gonnect.app.JsChatConnector")

JsChatConnector::JsChatConnector(const JsConnectorConfig &config, QObject *parent)
    : IChatProvider{ config.settingsGroup, parent }, m_config{ config }
{
    connect();
}

QUrl JsChatConnector::url() const
{
    return QString("http://localhost:%1/index.html").arg(m_tcpPort);
}

void JsChatConnector::connect()
{
    setupServer();

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

void JsChatConnector::setupServer()
{
    auto httpServer = new QHttpServer(this);
    httpServer->route("/<arg>", [this](const QUrl &url) {
        auto response = QHttpServerResponse::fromFile(m_config.url.toString() + "/" + url.path());

        // Wasm files need MIME type fixed
        if (url.path().endsWith(".wasm")) {
            auto headers = response.headers();
            headers.replaceOrAppend(QHttpHeaders::WellKnownHeader::ContentType, "application/wasm");
            response.setHeaders(headers);
        }

        return response;
    });

    auto tcpServer = new QTcpServer(this);
    if (!tcpServer->listen() || !httpServer->bind(tcpServer)) {
        qCCritical(lcJsChatConnector) << "Server failed to listen";
        return;
    }

    m_tcpPort = tcpServer->serverPort();
}

qsizetype JsChatConnector::indexOf(IChatRoom *chatRoom) const
{
    return m_rooms.indexOf(chatRoom);
}

IChatRoom *JsChatConnector::chatRoomByRoomId(const QString &roomId) const
{
    return qobject_cast<IChatRoom *>(m_roomLookup.value(roomId, nullptr));
}

void JsChatConnector::handleRecoveryKey(const QString &key)
{
    ChatConnectorManager::instance().saveRecoveryKey(m_config.settingsGroup, key);
    setIsSecretInitalized(true);
}

void JsChatConnector::handleAccessToken(const QString &value)
{
    ChatConnectorManager::instance().saveAccessToken(m_config.settingsGroup, value);
    m_config.accessToken = value;
    emit accessTokenChanged();
}

void JsChatConnector::addChatRoom(const QString &roomId, const QString name)
{
    createOrLookupChatRoom(roomId, name);
}

void JsChatConnector::addMessageEvent(const QString &eventId, const QString &roomId,
                                      const QString &senderId, const QString &message,
                                      const QDateTime &dateTime)
{
    ChatMessage::Flags flags;
    if (senderId == ownUserId()) {
        flags |= ChatMessage::Flag::OwnMessage;
    }

    auto room = createOrLookupChatRoom(roomId);
    room->addMessage(new JsChatMessageEvent(eventId, roomId, senderId, dateTime, message, flags));
}

void JsChatConnector::addImageEvent(const QString &eventId, const QString &roomId,
                                    const QString &senderId, const QString &imageUrl,
                                    const QDateTime &dateTime)
{
    m_events.append(new JsChatImageEvent(eventId, roomId, senderId, dateTime, imageUrl, this));
    emit chatEventAdded();
}

void JsChatConnector::addUser(const QString &userId, const QString &displayName)
{
    auto user = new JsChatUser(userId, displayName, this);
    m_users.insert(userId, user);
    emit chatUserAdded();
}

void JsChatConnector::updateRoomNotificationCount(const QString &roomId, quint16 count)
{
    auto room = m_roomLookup.value(roomId, nullptr);
    if (!room) {
        qCWarning(lcJsChatConnector) << "Unable to find room object for id" << roomId;
        return;
    }

    room->setUnreadNotificationCount(count);
}

void JsChatConnector::setIsSecretInitalized(bool value)
{
    if (m_isSecretInitalized != value) {
        m_isSecretInitalized = value;
        emit isSecretInitalizedChanged();
    }
}

JsChatRoom *JsChatConnector::createOrLookupChatRoom(const QString &roomId, const QString &roomName)
{
    JsChatRoom *room = m_roomLookup.value(roomId, nullptr);

    if (!room) {
        room = new JsChatRoom(roomId, roomName, this);
        m_rooms.append(room);
        m_roomLookup.insert(roomId, room);
        emit chatRoomAdded(m_rooms.length() - 1, room);

    } else if (!roomName.isEmpty()) {
        room->setName(roomName);
        emit chatRoomNameChanged(m_rooms.indexOf(room), room, roomName);
    }

    return room;
}
