#include "IpcDispatcher.h"
#include "IpcDispatcherAsserts.h"
#include "IpcProtoConversion.h"
#include "IpcChatRoom.h"
#include "ChatMessage.h"
#include "ChatUser.h"
#include "ChatMessageReaction.h"
#include "PersonCoinProvider.h"
#include "PluginManager.h"
#include "ChatMessageContentText.h"
#include "ChatMessageContentImage.h"
#include "ChatMessageContentFile.h"
#include "ChatMessageContentAudioFile.h"
#include "ChatMessageContentVideoFile.h"
#include "ChatMessageContentUserStateChange.h"
#include "ClipboardHelper.h"
#include "AppSettings.h"
#include "Notification.h"
#include "NotificationManager.h"
#include "ReadOnlyConfdSettings.h"
#include "ViewHelper.h"
#include "EnumTranslation.h"
#include "GlobalStateAggregator.h"
#include "PlatformSession.h"
#include "SelectionState.h"
#include "FileHelper.h"

#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QLoggingCategory>
#include <QStandardPaths>
#include <QUuid>

Q_LOGGING_CATEGORY(lcIpcDispatcher, "gonnect.app.chat.IpcDispatcher")

#define GONNECT_INITIAL_MESSAGE_LIMIT 42

using namespace de::gonicus::gonnect;

bool IpcDispatcher::checkConfig(const IpcConfig &config)
{
    if (config.userId.isEmpty()) {
        qCCritical(lcIpcDispatcher) << "User id is required";
        return false;
    }

    if (config.backendUrl.isEmpty()) {
        qCCritical(lcIpcDispatcher) << "Backend url is required";
        return false;
    }

    if (config.loginFlow == IpcConfig::LoginFlow::Unknown) {
        qCCritical(lcIpcDispatcher) << "Login flow is required";
        return false;
    }

    return true;
}

bool IpcDispatcher::requiresSecret(const IpcConfig &config)
{
    return config.loginFlow == IpcConfig::LoginFlow::Credentials;
}

IpcDispatcher::IpcDispatcher(const QString &settingsGroup, const IpcConfig &configInfo,
                             QObject *parent)
    : IChatProvider{ settingsGroup, parent }, m_configInfo{ configInfo }
{
    m_verificationTimeoutTimer.setSingleShot(true);
    m_verificationTimeoutTimer.callOnTimeout(
            this, []() { qCCritical(lcIpcDispatcher) << "Verification process timeout"; });

    m_unreadUpdateTimer.setSingleShot(true);
    m_unreadUpdateTimer.setInterval(200);
    m_unreadUpdateTimer.callOnTimeout(this, &IpcDispatcher::updateUnreadNotificationsCountImpl);

    // Setup id conversion
    if (!configInfo.idConvRegexpString.isEmpty() && !configInfo.idConvReplacementString.isEmpty()) {
        m_idConvRegex.setPattern(configInfo.idConvRegexpString);
        if (m_idConvRegex.isValid()) {
            m_useIdConversion = true;
        } else {
            qCCritical(lcIpcDispatcher)
                    << QString("The regular expression '%1' for '%2' is "
                               "invalid and will be ignored.")
                               .arg(configInfo.idConvRegexpString, settingsGroup);
        }
    }

    init();

    connect(this, &IpcDispatcher::isConnectedChanged, this, &IpcDispatcher::onLoggedInChanged);

    connect(this, &IpcDispatcher::connectionStateChanged, this, &IpcDispatcher::updateConnected);
    connect(this, &IpcDispatcher::initializedChanged, this, &IpcDispatcher::updateConnected);
    connect(this, &IpcDispatcher::capabilitiesInitializedChanged, this,
            &IpcDispatcher::updateConnected);

    connect(this, &IChatProvider::chatRoomAdded, this, &IpcDispatcher::updateHasFavoriteRooms);
    connect(this, &IChatProvider::chatRoomAdded, this,
            &IpcDispatcher::updateUnreadNotificationsCount);
    connect(this, &IChatProvider::chatRoomRemoved, this, &IpcDispatcher::updateHasFavoriteRooms);
    connect(this, &IChatProvider::chatRoomRemoved, this,
            &IpcDispatcher::updateUnreadNotificationsCount);

    updateConnected();
}

IpcDispatcher::~IpcDispatcher()
{
    m_ipc.stop();

    for (auto l : std::as_const(m_chatNotifications)) {
        delete l;
    }
    m_chatNotifications.clear();

    qDeleteAll(m_userRoomStateCache.values());
    m_userRoomStateCache.clear();
}

IChatProvider::Capabilities IpcDispatcher::capabilities() const
{
    // clang-format off
    using CAP = IChatProvider::Capability;
    static const IChatProvider::Capabilities s_capabilties =
            CAP::EditMessage
            | CAP::RemoveMessage
            | CAP::MessageRelations
            | CAP::Reactions
            | CAP::UploadFile
            | CAP::UploadMedia
            | CAP::Markdown;
    return s_capabilties;
    // clang-format on
}

void IpcDispatcher::loginWithCredentials(const QString &userId, const QString &secret)
{
    LoginUsernamePasswordRequest credReq;
    credReq.setUsername(userId);
    credReq.setPassword(secret);

    sendRequest([&](RequestContainer &req) { req.setLoginUsernamePasswordRequest(credReq); });
}

void IpcDispatcher::loginWithSSO(const QString &identityProvider)
{
    Q_ASSERT(identityProvider.isEmpty() || m_identityProviders.contains(identityProvider));

    // An empty string means that the default provider must be used
    if (!identityProvider.isEmpty() && !m_identityProviders.contains(identityProvider)) {
        qCCritical(lcIpcDispatcher)
                << QString("The identity provider '%1' is unknown. Please use one of these: %2")
                           .arg(identityProvider, m_identityProviders.join(", "));
        return;
    }

    LoginSSORequest ssoLoginReq;
    ssoLoginReq.setIdentityProvider(identityProvider);
    sendRequest([&](RequestContainer &req) { req.setLoginSSORequest(ssoLoginReq); });
}

void IpcDispatcher::sendMessage(const QString &roomId, const QString &text,
                                const QString &relatedMessageId)
{
    const auto *chatRoom = chatRoomByRoomId(roomId);
    if (!chatRoom) {
        qCCritical(lcIpcDispatcher) << "Unable to find room with id" << roomId << "- aborting";
        return;
    }

    // Collect user ids of room users
    const auto &users = chatRoom->chatUsers();
    QSet<QString> userIds;
    QStringList mentionedUserIds;
    userIds.reserve(users.size());
    mentionedUserIds.reserve(users.size());

    for (const auto *user : users) {
        userIds.insert(user->id());
    }

    // Collect mentioned ids
    const auto splitted = text.split(QChar(QChar::SpecialCharacter::Space));
    for (const auto &word : splitted) {
        if (userIds.contains(word)) {
            mentionedUserIds.append(word);
        }
    }

    // Assemble request
    MessageSendRequest msgReq;
    msgReq.setRoomId(roomId);

    if (!relatedMessageId.isEmpty()) {
        msgReq.setRelatedMessageId(relatedMessageId);
    }

    MessageContentText content;
    content.setContent(text);

    msgReq.setText(content);
    msgReq.setMentionedUserIds(mentionedUserIds);
    sendRequest([&](RequestContainer &req) { req.setMessageSendRequest(msgReq); });
}

void IpcDispatcher::sendTypingPing(const QString &roomId)
{
    RoomTypingRequest typingReq;
    typingReq.setRoomId(roomId);
    sendRequest([&](RequestContainer &req) { req.setRoomTypingRequest(typingReq); }, false);
}

void IpcDispatcher::sendFile(const QString &roomId, const QString &filePath,
                             const QString &originalFileName)
{
    if (!chatRoomByRoomId(roomId)) {
        qCCritical(lcIpcDispatcher) << "Unable to find room with id" << roomId << "- aborting";
        return;
    }

    MessageSendRequest msgReq;
    msgReq.setRoomId(roomId);

    MessageContentFile content;
    content.setFilePath(makeRelativeToDataRootPath(filePath));

    if (!originalFileName.isEmpty()) {
        content.setFileName(originalFileName);
    }

    msgReq.setFile(content);
    sendRequest([&](RequestContainer &req) { req.setMessageSendRequest(msgReq); });
}

void IpcDispatcher::respondToInvitation(const QString &roomId, bool acceptInvitation)
{
    InvitedReply reply;
    reply.setRoomId(roomId);
    reply.setAccepted(acceptInvitation);
    sendRequest([&](RequestContainer &req) { req.setInvitedReply(reply); }, false);
}

void IpcDispatcher::markAsRead(const QString &roomId)
{
    Q_ASSERT(!roomId.isEmpty());

    RoomMarkAsReadRequest markReq;
    markReq.setRoomId(roomId);
    sendRequest([&](RequestContainer &req) { req.setRoomMarkAsReadRequest(markReq); }, false);
}

void IpcDispatcher::loadMessages(IChatRoom *chatRoom)
{
    Q_CHECK_PTR(chatRoom);

    if (chatRoom->isLoadingMessageHistory()) {
        return;
    }

    chatRoom->setIsLoadingMessageHistory(true);

    RoomMessagesRequest msgReq;
    msgReq.setRoomId(chatRoom->id());
    msgReq.setLimit(GONNECT_INITIAL_MESSAGE_LIMIT);
    msgReq.setOrder(MessagesOrderGadget::MessagesOrder::Backward);

    const auto &existingMessages = chatRoom->chatMessages();
    if (!existingMessages.isEmpty()) {
        msgReq.setFromMessageId(existingMessages.first()->eventId());
    }

    const auto tag =
            sendRequest([&](RequestContainer &req) { req.setRoomMessagesRequest(msgReq); });
    m_roomListTags.insert(tag, chatRoom->id());
}

void IpcDispatcher::loadSingleMessage(const QString &roomId, const QString &messageId)
{
    GONNECT_ASSERT(!roomId.isEmpty(), "roomId must not be empty")
    GONNECT_ASSERT(!messageId.isEmpty(), "messageId must not be empty")

    if (m_failedMessageIds.contains(messageId)) {
        qCInfo(lcIpcDispatcher) << "Message" << messageId
                                << "has already failed to load - skipping.";
        return;
    }

    const auto it = std::find(m_singleMessageTags.cbegin(), m_singleMessageTags.cend(), messageId);
    if (it != m_singleMessageTags.cend()) {
        qCInfo(lcIpcDispatcher)
                << "Message" << messageId
                << "has already been requested, but response is pending - ignoring.";
        return;
    }

    MessageRequest msgReq;
    msgReq.setRoomId(roomId);
    msgReq.setMessageId(messageId);

    const auto tag = sendRequest([&](RequestContainer &req) { req.setMessageRequest(msgReq); });
    m_singleMessageTags.insert(tag, messageId);
}

qsizetype IpcDispatcher::chatRoomsCount()
{
    return m_rooms.length();
}

IChatRoom *IpcDispatcher::chatRoomByIndex(qsizetype index)
{
    if (index < 0 || index >= m_rooms.length()) {
        qCWarning(lcIpcDispatcher) << "Index" << index << "out of bounds for rooms which has"
                                   << m_rooms.length() << "elements";
        return nullptr;
    }
    return m_rooms.at(index);
}

void IpcDispatcher::requestRemoveMessage(const QString &roomId, const QString &messageId)
{
    MessageRemoveRequest removeReq;
    removeReq.setRoomId(roomId);
    removeReq.setMessageId(messageId);

    sendRequest([&](RequestContainer &req) { req.setMessageRemoveRequest(removeReq); });
}

void IpcDispatcher::requestEditMessage(const QString &roomId, const QString &messageId,
                                       const QString &newContent)
{
    const auto content = newContent.trimmed();

    if (content.isEmpty()) {
        qCWarning(lcIpcDispatcher) << "editing a message and setting an empty string as new "
                                      "content does not make sense";
    }

    MessageChangeRequest changeReq;
    changeReq.setRoomId(roomId);
    changeReq.setMessageId(messageId);

    MessageContentText msgContent;
    msgContent.setContent(content);
    changeReq.setText(msgContent);

    sendRequest([&](RequestContainer &req) { req.setMessageChangeRequest(changeReq); });
}

/// Convert the numeric confd log level to the log level argument of the headless client.
static QString logLevelArgument(uint configLogLevel)
{
    switch (configLogLevel) {
    case 1:
        return "error";
    case 2:
        return "warn";
    case 3:
        return "info";
    case 4:
    case 5:
    case 6:
        return "debug";
    default:
        return configLogLevel >= 7 ? "trace" : "off";
    }
}

void IpcDispatcher::init()
{
    connect(
            &m_ipc, &IpcInterface::isRunningChanged, this,
            [this]() {
                sendInitialInitializationRequest();
                m_isInitialized = true;
                Q_EMIT initializedChanged();
            },
            Qt::SingleShotConnection);

    connect(&m_ipc, &IpcInterface::blobReceived, this, &IpcDispatcher::onBlobReceived);

    const auto plugin = PluginManager::instance().pluginByType("chat-ipc");

    if (!plugin) {
        qCCritical(lcIpcDispatcher) << "Unable to find matrix-ipc plugin";
        return;
    }

    m_ipc.setBinaryPath(plugin->binPath);
    QStringList args;

    // Convert and set log level
    ReadOnlyConfdSettings settings;
    args.append({ "--log-level", logLevelArgument(settings.value("logging/level", 2).toUInt()) });

    const auto logFilePath = FileHelper::instance().makeLogFilePath(plugin->displayName);
    if (logFilePath.isEmpty()) {
        qCWarning(lcIpcDispatcher)
                << "Unable to create valid log file path - --log-file-path will not be set";
    } else {
        args.append({ "--log-file-path", logFilePath });
    }

    // Set positional arguments
    args.append({ m_ipc.fullRequestServerName(), m_ipc.fullResponseServerName() });

    m_ipc.setBinaryArguments(args);
    m_ipc.start();
}

void IpcDispatcher::sendRequestContainer(RequestContainer &container, quint32 timeoutSeconds)
{
    if (!m_ipc.isRunning()) {
        qWarning() << "Local socket is not ready - aborting";
        return;
    }

    // Check which contents are set
    const QStringList types = contentTypes(container);

    if (!types.length()) {
        qCCritical(lcIpcDispatcher) << "The request container has no content - sending is aborted";
        return;
    }

    if (types.length() > 1) {
        qCCritical(lcIpcDispatcher) << QString("The request container should contain exactly one "
                                               "content but has %1: %2 - sending is aborted")
                                               .arg(types.length())
                                               .arg(types.join(", "));
        return;
    }

    const auto tag = container.tag();
    const QString tagDbg = tag > 0 ? QString("(tag: %1)").arg(tag) : "";
    qCInfo(lcIpcDispatcher).noquote() << "Sending request with content" << types.first() << tagDbg;

    // Handle tag
    if (timeoutSeconds && tag > 0) {

        auto timer = new QTimer(this);
        timer->setSingleShot(true);
        timer->setInterval(timeoutSeconds * 1000);
        timer->callOnTimeout(this, [timer, tag, timeoutSeconds, this]() {
            m_timeoutTimers.remove(tag);
            markSingleMessageFailed(tag);
            timer->deleteLater();

            qCCritical(lcIpcDispatcher) << "IPC request with tag" << tag << "has timeout after"
                                        << timeoutSeconds << "seconds";
        });

        m_timeoutTimers.insert(tag, timer);
        timer->start();
    }

    // Serialize and send request
    m_ipc.writeData(container.serialize(&m_protoSerializer));
}

void IpcDispatcher::markSingleMessageFailed(quint64 tag)
{
    const auto messageId = m_singleMessageTags.take(tag);
    if (!messageId.isEmpty()) {
        m_failedMessageIds.insert(messageId);
    }
}

bool IpcDispatcher::hasOwnUserMention(const ChatMessage &message) const
{
    if (const auto *textContent = qobject_cast<const ChatMessageContentText *>(message.content())) {

        const auto mentions = message.mentionedUsers();
        for (const auto *user : mentions) {
            if (user->id() == ownUserId()) {
                return true;
            }
        }

        if (textContent->isSimpleText()) {
            return textContent->simpleText().contains(u"@room");
        } else {
            const auto parts = textContent->contentParts();
            for (const auto part : parts) {
                if (!part->isCode() && part->text().contains(u"@room")) {
                    return true;
                }
            }
        }
    }

    return false;
}

IpcChatRoom *IpcDispatcher::ipcChatRoomById(const QString &roomId) const
{
    return m_roomLookup.value(roomId, nullptr);
}

QString IpcDispatcher::ensureDataFolderExists()
{
    Q_ASSERT(!m_configInfo.configHash.isEmpty());

    if (m_dataFolderPath.isEmpty()) {
        m_dataFolderPath =
                QString("%1/chats/%2")
                        .arg(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation),
                             m_configInfo.configHash);
    }

    QDir dir;
    dir.mkpath(m_dataFolderPath);

    return m_dataFolderPath;
}

QString IpcDispatcher::uploadFolderPath() const
{
    const QString uploadFolderPath = makeDataRootPath("uploads").mid(7);
    QDir dir;
    if (!dir.mkpath(uploadFolderPath)) {
        qCCritical(lcIpcDispatcher) << "Unable to create directory" << uploadFolderPath;
        return "";
    }
    return uploadFolderPath;
}

QString IpcDispatcher::makeDataRootPath(const QString &subPath) const
{
    const QFileInfo path(QString("file://%1/%2").arg(m_dataFolderPath, subPath));
    return path.filePath();
}

QString IpcDispatcher::makeRelativeToDataRootPath(const QString &path) const
{
    QString s(path);

    static const QString fileProtocol("file://");

    if (s.startsWith(fileProtocol)) {
        s.remove(0, fileProtocol.length());
    }

    if (s.startsWith(m_dataFolderPath)) {
        s.remove(0, m_dataFolderPath.length());
    }

    if (s.startsWith('/')) {
        s.remove(0, 1);
    }

    return s;
}

IpcChatRoom *IpcDispatcher::ipcDirectChatRoomForUser(const ChatUser *user) const
{
    if (!user) {
        return nullptr;
    }

    const auto userId = user->id();

    for (const auto room : std::as_const(m_rooms)) {
        if (room->isDirectChat() && room->isUserMemberOfRoom(userId)) {
            return room;
        }
    }

    return nullptr;
}

void IpcDispatcher::setIsInVerificationProcess(bool value)
{
    if (m_isInVerificationProcess != value) {
        m_isInVerificationProcess = value;
        Q_EMIT isInVerificationProcessChanged();
    }
}

void IpcDispatcher::setIsDeviceVerified(bool value)
{
    if (m_isDeviceVerified != value) {
        m_isDeviceVerified = value;
        Q_EMIT isDeviceVerifiedChanged();
    }
}

std::pair<QString, QString> IpcDispatcher::notificationTitleAndMessage(ChatMessage *messageObj,
                                                                       IChatRoom *chatRoom) const
{
    QString title;
    QString message;
    const QString senderName = messageObj->nickName();

    if (const auto stateContent =
                qobject_cast<ChatMessageContentUserStateChange *>(messageObj->content())) {
        QString otherName("Someone");
        if (const auto user = m_users.value(stateContent->affectedUserId(), nullptr)) {
            otherName = user->computedName();
        }
        title = EnumTranslation::instance().userStateChange(stateContent->state(), otherName);

    } else if (qobject_cast<ChatMessageContentImage *>(messageObj->content())) {
        if (chatRoom->isDirectChat()) {
            title = tr("Image sent by %1").arg(senderName);
        } else {
            title = tr("[%1] Image sent by %2").arg(chatRoom->name(), senderName);
        }
    } else if (const auto audioContent =
                       qobject_cast<ChatMessageContentAudioFile *>(messageObj->content())) {

        if (chatRoom->isDirectChat()) {
            title = tr("Audio file sent by %1").arg(senderName);
        } else {
            title = tr("[%1] Audio file sent by %2").arg(chatRoom->name(), senderName);
        }
        message = audioContent->fileName();
    } else if (const auto videoContent =
                       qobject_cast<ChatMessageContentVideoFile *>(messageObj->content())) {

        if (chatRoom->isDirectChat()) {
            title = tr("Video file sent by %1").arg(senderName);
        } else {
            title = tr("[%1] Video file sent by %2").arg(chatRoom->name(), senderName);
        }
        message = videoContent->fileName();
    } else if (const auto fileContent =
                       qobject_cast<ChatMessageContentFile *>(messageObj->content())) {

        if (chatRoom->isDirectChat()) {
            title = tr("File sent by %1").arg(senderName);
        } else {
            title = tr("[%1] File sent by %2").arg(chatRoom->name(), senderName);
        }
        message = fileContent->fileName();
    } else if (const auto textContent =
                       qobject_cast<ChatMessageContentText *>(messageObj->content())) {

        if (chatRoom->isDirectChat()) {
            title = tr("Message from %1").arg(senderName);
        } else {
            title = tr("[%1] Message from %2").arg(chatRoom->name(), senderName);
        }
        message = textContent->simpleText();
    }

    return { title, message };
}

void IpcDispatcher::makeNotificationNewMessage(ChatMessage *messageObj)
{
    // Check if notifications should be send at all
    if (!messageObj || !shallSendDesktopNotification() || messageObj->fromId() == ownUserId()) {
        return;
    }

    // Check if not active and in foreground anyway
    auto &selectionState = SelectionState::instance();
    if (selectionState.isMainWindowActive()
        && selectionState.selectedPage().type == MainPageSelection::PageType::Chats) {

        if (const auto *selectedChatRoom = selectionState.selectedChatRoom();
            selectedChatRoom && selectedChatRoom == messageObj->chatRoom()) {
            return;
        }
    }

    // Check global and room-specific notification settings
    using NotificationSetting = NotificationSetting::Setting;
    auto notificationSetting = m_notificationSetting;
    IChatRoom *chatRoom = q_check_ptr(messageObj->chatRoom());

    // Override in room
    const auto roomNotificationSetting = chatRoom->roomSettings().notificationSetting;
    if (roomNotificationSetting != NotificationSetting::None) {
        notificationSetting = roomNotificationSetting;
    }

    if (notificationSetting == NotificationSetting::Mute
        || notificationSetting == NotificationSetting::None) {
        return;
    }

    if (notificationSetting == NotificationSetting::MentionsAndKeywords
        && !hasOwnUserMention(*messageObj)) {
        return;
    }

    const auto [title, message] = notificationTitleAndMessage(messageObj, chatRoom);

    auto notification =
            new Notification(title, message, Notification::Priority::normal, true, this);

    QString avatarPath = chatRoom->avatarPath();
    if (avatarPath.isEmpty() && chatRoom->isDirectChat()) {
        if (const auto other = chatRoom->otherUser()) {
            avatarPath = other->avatarPath();
        }
    }

    if (avatarPath.startsWith("file://")) {
        avatarPath = avatarPath.mid(7);
    } else if (avatarPath.isEmpty()) {
        const auto initials = ViewHelper::instance().initials(messageObj->nickName());
        PersonCoinProvider personCoinProvider;
        avatarPath = personCoinProvider.makePath(initials, 48);

        if (!QFileInfo::exists(avatarPath)) {
            QSize size;
            personCoinProvider.requestImage(initials, &size, QSize(48, 48));
        }
    }

    notification->setIcon(avatarPath);
    notification->setRoundedIcon(true);
    notification->setDefaultAction("show");
    notification->setDisplayHint(Notification::tray | Notification::hideContentOnLockScreen);

    const QString roomId = chatRoom->id();

    QObject::connect(notification, &Notification::actionInvoked, this,
                     [this, roomId](QString, QVariantList) {
                         qCDebug(lcIpcDispatcher)
                                 << "showChatRoom action invoked per desktop notification for room"
                                 << roomId;
                         Q_EMIT ViewHelper::instance().showChatRoom(this, roomId);
                     });

    if (!m_chatNotifications.contains(chatRoom)) {
        auto l = new QList<Notification *>;
        m_chatNotifications.insert(chatRoom, l);
    }
    m_chatNotifications.value(chatRoom)->append(notification);

    NotificationManager::instance().add(notification);
}

void IpcDispatcher::removeNotificationsForRoom(IChatRoom *room)
{

    if (!room || !m_chatNotifications.contains(room)) {
        return;
    }

    auto l = m_chatNotifications.value(room);
    m_chatNotifications.remove(room);
    qDeleteAll(*l);
    delete l;
}

bool IpcDispatcher::shallSendDesktopNotification()
{
    if (PlatformSession::instance().isScreenShareActive()) {
        return false;
    }
    AppSettings settings;
    return settings.value("generic/jitsiChatAsNotifications", true).toBool();
}

void IpcDispatcher::onBlobReceived(const QByteArray data)
{
    ResponseContainer response;
    if (!response.deserialize(&m_protoSerializer, data)) {
        qCritical() << "Error on deserializing protobuf message:"
                    << m_protoSerializer.lastErrorString();
        qFatal("Deserializing of protobuf payload failed");
    }

    processResponse(response);
}

void IpcDispatcher::updateConnected()
{
    const bool isConnected = isInitialized() && areCapabilitesInitialized()
            && (connectionState() == ConnectionState::LoggedIn);

    if (m_isConnected != isConnected) {
        m_isConnected = isConnected;
        Q_EMIT isConnectedChanged();
    }
}

void IpcDispatcher::updateHasFavoriteRooms()
{
    bool hasFav = false;

    for (auto *room : std::as_const(m_rooms)) {
        if (room->isFavorite()) {
            hasFav = true;
            break;
        }
    }

    if (m_hasFavoriteRooms != hasFav) {
        m_hasFavoriteRooms = hasFav;
        Q_EMIT hasFavoriteRoomsChanged();
    }
}

void IpcDispatcher::sendInitialInitializationRequest()
{
    // Initial initialization request
    InitializationRequest initReq;
    initReq.setBackendUrl(m_configInfo.backendUrl.toString());
    initReq.setDataRootPath(ensureDataFolderExists());
    initReq.setEncryptionSecret(m_configInfo.encryptionSecret);
    initReq.setPersistentStorageSecret(m_configInfo.persistentStorageSecret);
    initReq.setDeviceDisplayName(m_configInfo.displayName);
    sendRequest([&](RequestContainer &req) { req.setInitializationRequest(initReq); }, false);
}

void IpcDispatcher::onLoggedInChanged()
{
    if (!isConnected() && m_globalPresenceStateContext) {
        m_globalPresenceStateContext->deleteLater();
        m_globalPresenceStateContext = nullptr;

    } else if (isConnected() && !m_globalPresenceStateContext) {
        m_globalPresenceStateContext = new QObject(this);

        auto &glob = GlobalStateAggregator::instance();
        connect(&glob, &GlobalStateAggregator::presenceStateChanged, m_globalPresenceStateContext,
                [this]() { forwardOwnPresenceState(); });
        connect(&glob, &GlobalStateAggregator::statusTextChanged, m_globalPresenceStateContext,
                [this]() { forwardOwnPresenceState(); });
        forwardOwnPresenceState();

        // Request global settings
        sendRequest([](RequestContainer &req) {
            req.setGlobalSettingsRequest(GlobalSettingsRequest());
        });
    }
}

void IpcDispatcher::forwardOwnPresenceState()
{

    if (isConnected()) {
        UserStatus statusReq;
        auto &glob = GlobalStateAggregator::instance();

        if (!glob.statusText().isEmpty()) {
            statusReq.setStatusMessage(glob.statusText());
        }

        switch (glob.presenceState()) {

        case PresenceState::State::Unknown:
        case PresenceState::State::Offline:
            statusReq.setState(PresenceStateGadget::PresenceState::Offline);
            break;

        case PresenceState::State::Away:
        case PresenceState::State::Busy:
            statusReq.setState(PresenceStateGadget::PresenceState::Away);
            break;

        case PresenceState::State::Available:
        case PresenceState::State::Ringing:
            statusReq.setState(PresenceStateGadget::PresenceState::Online);
            break;
        }

        sendRequest([&](RequestContainer &req) { req.setUserStatusSetOwnRequest(statusReq); });
    }
}

void IpcDispatcher::updateUnreadNotificationsCount()
{
    m_unreadUpdateTimer.start();
}

void IpcDispatcher::updateUnreadNotificationsCountImpl()
{
    qsizetype count = 0;

    for (auto *room : std::as_const(m_rooms)) {
        if (room->ownUserJoinState() == IChatRoom::UserRoomState::Joined) {
            count += room->notificationCount();
        }
    }

    setUnreadNotificationsCount(count);
}

QStringList IpcDispatcher::contentTypes(const de::gonicus::gonnect::RequestContainer &container)
{
    const auto &meta = container.staticMetaObject;
    QStringList result;

    for (int i = meta.propertyOffset(); i < meta.propertyCount(); ++i) {
        const auto property = meta.property(i);
        if (QString(property.name()).startsWith("has") && property.isReadable()
            && property.typeName() == QString("bool")) {

            if (property.readOnGadget(&container).toBool()) {
                result.append(QString(property.name()).mid(3));
            }
        }
    }

    return result;
}

void IpcDispatcher::login()
{
    if (m_configInfo.loginFlow == IpcConfig::LoginFlow::Credentials) {
        loginWithCredentials(m_configInfo.userId, m_configInfo.secret);

    } else if (m_configInfo.loginFlow == IpcConfig::LoginFlow::SSO) {
        loginWithSSO(m_configInfo.identityProviderId);

    } else {
        qCCritical(lcIpcDispatcher)
                << "Unimplemented login flow:" << static_cast<quint64>(m_configInfo.loginFlow);
    }
}

bool IpcDispatcher::hasFavoriteRooms() const
{
    return m_hasFavoriteRooms;
}

qsizetype IpcDispatcher::indexOf(IChatRoom *chatRoom) const
{
    return m_rooms.indexOf(chatRoom);
}

IChatRoom *IpcDispatcher::chatRoomByRoomId(const QString &roomId) const
{
    return m_roomLookup.value(roomId, nullptr);
}

QString IpcDispatcher::chatRoomIdForUser(const ChatUser *user) const
{
    if (!user) {
        return "";
    }

    const auto userId = user->id();

    for (const auto room : std::as_const(m_rooms)) {
        const auto &roomUsers = room->chatUsers();
        if (roomUsers.length() == 2 && room->isUserMemberOfRoom(userId)
            && room->isUserMemberOfRoom(m_configInfo.userId)) {
            return room->id();
        }
    }
    return "";
}

QString IpcDispatcher::chatRoomIdForUser(const QString &userId) const
{
    const auto user = m_users.value(userId, nullptr);
    if (user) {
        return chatRoomIdForUser(user);
    } else {
        qCCritical(lcIpcDispatcher) << "Unable to find user object for id" << userId;
        return "";
    }
}

QString IpcDispatcher::searchChatUser(const QString &searchPhrase)
{
    UserSearchRequest searchReq;
    searchReq.setQuery(searchPhrase);
    searchReq.setLimit(50);

    const auto tag =
            sendRequest([&](RequestContainer &req) { req.setUserSearchRequest(searchReq); });
    return QString::number(tag);
}

QList<const ChatUser *> IpcDispatcher::users() const
{
    return m_userList;
}

ChatUser *IpcDispatcher::userById(const QString &userId) const
{
    return m_users.value(userId, nullptr);
}

void IpcDispatcher::inviteUsers(const QString &roomId, const QList<QString> &userIds,
                                const QString &text)
{
    InvitationRequest inv;
    inv.setRoomId(roomId);
    inv.setInvitees(userIds);
    if (!text.isEmpty()) {
        inv.setInvitationText(text);
    }
    sendRequest([&](RequestContainer &req) { req.setInvitationRequest(inv); }, false);
}

QString IpcDispatcher::searchPublicRoomRequest(const QString &searchText,
                                               const QString &offsetToken, quint32 limit)
{
    PublicRoomListRequest listReq;
    if (!searchText.isEmpty()) {
        listReq.setGenericSearchTerm(searchText);
    }
    if (!offsetToken.isEmpty()) {
        listReq.setSince(offsetToken);
    }
    if (limit) {
        listReq.setLimit(limit);
    }

    const auto tag =
            sendRequest([&](RequestContainer &req) { req.setPublicRoomListRequest(listReq); });
    return QString::number(tag);
}

QString IpcDispatcher::requestDirectRoomCreation(const QString &userId, const QString &name,
                                                 const QString &avatarPath)
{
    if (userId.isEmpty()) {
        qCCritical(lcIpcDispatcher) << "Assertion failed: the user id must not be empty";
        return "";
    }

    RoomCreateDirectRequest createReq;
    createReq.setInvitee(userId);
    if (!name.isEmpty()) {
        createReq.setDisplayName(name);
    }
    if (!avatarPath.isEmpty()) {
        createReq.setAvatarPath(makeRelativeToDataRootPath(avatarPath));
    }

    const auto tag =
            sendRequest([&](RequestContainer &req) { req.setRoomCreateDirectRequest(createReq); });
    return QString::number(tag);
}

QString IpcDispatcher::requestGroupRoomCreation(const QStringList &userIds,
                                                const IChatRoom::JoinRule joinRule,
                                                const QString &name, const QString &avatarPath)
{
    if (name.isEmpty()) {
        qCCritical(lcIpcDispatcher) << "Assertion failed: the name must not be empty";
        return "";
    }

    RoomCreateGroupRequest createReq;
    createReq.setDisplayName(name);
    createReq.setJoinRule(joinRuleGonnectToGrpc(joinRule));
    if (!userIds.isEmpty()) {
        createReq.setInvitees(userIds);
    }
    if (!avatarPath.isEmpty()) {
        createReq.setAvatarPath(makeRelativeToDataRootPath(avatarPath));
    }

    const auto tag =
            sendRequest([&](RequestContainer &req) { req.setRoomCreateGroupRequest(createReq); });
    return QString::number(tag);
}

void IpcDispatcher::requestRoomChange(IChatRoom *chatRoom, const QString &name,
                                      IChatRoom::JoinRule joinRule, const QString &avatarPath)
{
    GONNECT_ASSERT(chatRoom, "invalid chat room pointer");

    RoomChangeRequest changeReq;
    changeReq.setRoomId(chatRoom->id());

    bool hasChanged = false;

    if (chatRoom->name() != name) {
        changeReq.setDisplayName(name);
        hasChanged = true;
    }

    if (chatRoom->avatarPath() != avatarPath) {
        changeReq.setAvatarPath(makeRelativeToDataRootPath(avatarPath));
        hasChanged = true;
    }

    if (joinRule != IChatRoom::JoinRule::Unknown) {
        if (chatRoom->joinRule() != joinRule) {
            changeReq.setJoinRule(joinRuleGonnectToGrpc(joinRule));
            hasChanged = true;
        }
    }

    GONNECT_ASSERT(hasChanged, "requestRoomChange has been called although nothing has changed");

    sendRequest([&](RequestContainer &req) { req.setRoomChangeRequest(changeReq); });
}

void IpcDispatcher::requestToggleRoomFavorite(IChatRoom *chatRoom)
{
    GONNECT_ASSERT(chatRoom, "chatRoom pointer must not be nullptr")

    RoomChangeRequest changeReq;
    changeReq.setRoomId(chatRoom->id());
    changeReq.setIsFavorite(!chatRoom->isFavorite());

    sendRequest([&](RequestContainer &req) { req.setRoomChangeRequest(changeReq); });
}

void IpcDispatcher::joinRoomRequest(const QString &roomId)
{
    GONNECT_ASSERT(!roomId.isEmpty(), "roomId must not be empty")

    RoomJoinRequest joinReq;
    joinReq.setRoomId(roomId);

    sendRequest([&](RequestContainer &req) { req.setRoomJoinRequest(joinReq); }, false);
}

void IpcDispatcher::knockRoomRequest(const QString &roomId, const QString &message)
{
    GONNECT_ASSERT(!roomId.isEmpty(), "roomId must not be empty")

    RoomKnockRequest knockReq;
    knockReq.setRoomId(roomId);

    if (!message.isEmpty()) {
        knockReq.setMessage(message);
    }

    sendRequest([&](RequestContainer &req) { req.setRoomKnockRequest(knockReq); }, false);
}

void IpcDispatcher::requestRoomLeave(const QString &roomId)
{
    GONNECT_ASSERT(!roomId.isEmpty(), "roomId must not be empty")

    RoomLeaveRequest leaveReq;
    leaveReq.setRoomId(roomId);

    sendRequest([&](RequestContainer &req) { req.setRoomLeaveRequest(leaveReq); });
}

void IpcDispatcher::requestUser(const QString &userId)
{
    GONNECT_ASSERT(!userId.isEmpty(), "userId must not be empty")

    // Only request each user once
    if (m_requestedUserIds.contains(userId)) {
        return;
    }

    m_requestedUserIds.insert(userId);

    // Actual request
    UserRequest userReq;
    userReq.setUserId(userId);

    sendRequest([&](RequestContainer &req) { req.setUserRequest(userReq); });
}

void IpcDispatcher::addReaction(const QString &roomId, const QString &messageId,
                                const QString &reaction)
{
    GONNECT_ASSERT(!roomId.isEmpty(), "roomId must not be empty")
    GONNECT_ASSERT(!messageId.isEmpty(), "messageId must not be empty")
    GONNECT_ASSERT(!reaction.isEmpty(), "reaction must not be empty")

    Reaction r;
    r.setRoomId(roomId);
    r.setMessageId(messageId);
    r.setReaction(reaction);

    sendRequest([&](RequestContainer &req) { req.setCreateReactionRequest(r); }, false);
}

void IpcDispatcher::retractReaction(const QString &roomId, const QString &messageId,
                                    const QString &reaction)
{
    GONNECT_ASSERT(!roomId.isEmpty(), "roomId must not be empty")
    GONNECT_ASSERT(!messageId.isEmpty(), "messageId must not be empty")
    GONNECT_ASSERT(!reaction.isEmpty(), "reaction must not be empty")

    Reaction r;
    r.setRoomId(roomId);
    r.setMessageId(messageId);
    r.setReaction(reaction);

    sendRequest([&](RequestContainer &req) { req.setRemoveReactionRequest(r); }, false);
}

void IpcDispatcher::toggleReaction(const QString &roomId, const QString &messageId,
                                   const QString &reaction)
{
    GONNECT_ASSERT(!roomId.isEmpty(), "roomId must not be empty")
    GONNECT_ASSERT(!messageId.isEmpty(), "messageId must not be empty")
    GONNECT_ASSERT(!reaction.isEmpty(), "reaction must not be empty")

    const auto *room = m_roomLookup.value(roomId, nullptr);
    GONNECT_ASSERT(room, "Unable to find room for id " + roomId)

    const auto *message = room->chatMessageById(messageId);
    GONNECT_ASSERT(message, "Unable to find message for id " + messageId)

    const auto ownId = ownUserId();
    const auto &reactions = message->reactions();
    for (const auto *reactionObj : reactions) {
        if (reactionObj->reaction() == reaction && reactionObj->isUser(ownId)) {
            retractReaction(roomId, messageId, reaction);
            return;
        }
    }

    addReaction(roomId, messageId, reaction);
}

QString IpcDispatcher::uploadFile(const QString &filePath)
{
    const QString uploadFolderPath = this->uploadFolderPath();
    if (uploadFolderPath.isEmpty()) {
        qCCritical(lcIpcDispatcher) << "Unable to obtain upload folder path - aborting upload";
        return "";
    }

    const QUrl sourceUrl(filePath);
    const auto nonUrlPath = sourceUrl.isLocalFile() ? sourceUrl.toLocalFile() : filePath;
    if (nonUrlPath.startsWith(uploadFolderPath)) {
        return QString("file://%1").arg(nonUrlPath);
    }

    QString suffix;
    if (filePath.contains('.')) {
        suffix = QString(".%1").arg(filePath.split('.').last());
    }

    const auto uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString newPath = QString("%1/%2%3").arg(uploadFolderPath, uuid, suffix);
    QFile sourceFile(nonUrlPath);

    if (!sourceFile.copy(newPath)) {
        const QFileDevice::FileError err = sourceFile.error();
        const QString errorMsg = sourceFile.errorString();
        qCCritical(lcIpcDispatcher) << "Unable to copy" << filePath << "to" << newPath
                                    << "code:" << err << "error message:" << errorMsg;
        return "";
    }

    return QString("file://%1").arg(newPath);
}

void IpcDispatcher::uploadImageFromClipboard(const QString &roomId)
{
    auto &clippy = ClipboardHelper::instance();
    if (clippy.hasImage()) {
        const QString uploadFolderPath = this->uploadFolderPath();
        if (uploadFolderPath.isEmpty()) {
            qCCritical(lcIpcDispatcher) << "Unable to obtain upload folder path - aborting upload";
            return;
        }

        const auto uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
        const QString uploadedPath = QString("%1/%2.webp").arg(uploadFolderPath, uuid);

        if (!clippy.image().save(uploadedPath)) {
            qCCritical(lcIpcDispatcher) << "Error while saving image from clipboard";
            return;
        }

        auto room = m_roomLookup.value(roomId, nullptr);
        if (!room) {
            qCCritical(lcIpcDispatcher) << "Unable to find room object for id" << roomId;
            return;
        }

        Q_EMIT clipboardImageUploaded(uploadedPath, room);
    }
}

void IpcDispatcher::requestRecoveryKeyVerification(const QString &key)
{
    GONNECT_ASSERT_HAS_VERIFICATION
    GONNECT_ASSERT_IS_NOT_IN_VERIFICATION_PROCESS

    setIsInVerificationProcess(true);

    RecoveryKeyVerificationRequest recoveryReq;
    recoveryReq.setRecoveryKey(key);
    sendRequest([&](RequestContainer &req) { req.setRecoveryKeyVerificationRequest(recoveryReq); },
                false);
}

void IpcDispatcher::acceptVerification()
{
    GONNECT_ASSERT_HAS_VERIFICATION
    GONNECT_ASSERT_IS_IN_VERIFICATION_PROCESS
    GONNECT_ASSERT(!m_verificationFlowId.isEmpty(),
                   "Need a verification flow id for accepting the verification")

    CrossSigningConfirmRequest acceptReq;
    acceptReq.setVerificationFlowId(m_verificationFlowId);
    sendRequest([&](RequestContainer &req) { req.setCrossSigningConfirmRequest(acceptReq); },
                false);
}

void IpcDispatcher::requestVerificationAbort()
{
    GONNECT_ASSERT_HAS_VERIFICATION
    GONNECT_ASSERT_IS_IN_VERIFICATION_PROCESS
    GONNECT_ASSERT(!m_verificationFlowId.isEmpty(),
                   "Need a verification flow id for accepting the verification")

    VerificationAbortRequest abortReq;
    abortReq.setVerificationFlowId(m_verificationFlowId);
    sendRequest([&](RequestContainer &req) { req.setVerificationAbortRequest(abortReq); }, false);
}

void IpcDispatcher::requestCrossSigningStart()
{
    GONNECT_ASSERT_HAS_VERIFICATION
    GONNECT_ASSERT_IS_NOT_IN_VERIFICATION_PROCESS

    CrossSigningStartRequest startReq;
    startReq.setSupportedMethods({ CrossSigningMethodGadget::CrossSigningMethod::SasString,
                                   CrossSigningMethodGadget::CrossSigningMethod::SasSymbol });

    if (!m_verificationFlowId.isEmpty()) {
        startReq.setVerificationFlowId(m_verificationFlowId);
    }

    sendRequest([&](RequestContainer &req) { req.setCrossSigningStartRequest(startReq); }, false);
}

void IpcDispatcher::selectCrossSigningMethod(CrossSigningSecret::CrossSigningMethod method)
{
    GONNECT_ASSERT_HAS_VERIFICATION
    GONNECT_ASSERT_IS_IN_VERIFICATION_PROCESS
    GONNECT_ASSERT(!m_verificationFlowId.isEmpty(),
                   "Need a verification flow id for accepting the verification")

    CrossSigningMethodSelectedRequest selectReq;
    selectReq.setVerificationFlowId(m_verificationFlowId);
    selectReq.setSelectedMethod(crossSigningMethodReConv(method));
    sendRequest([&](RequestContainer &req) { req.setCrossSigningMethodSelectedRequest(selectReq); },
                false);
}

#undef GONNECT_INITIAL_MESSAGE_LIMIT
