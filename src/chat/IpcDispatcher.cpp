#include "IpcDispatcher.h"
#include "IpcChatRoom.h"
#include "ChatMessage.h"
#include "ChatUser.h"
#include "ChatMessageReaction.h"
#include "PersonCoinProvider.h"
#include "PluginManager.h"
#include "AddressBook.h"
#include "PublicChatRoom.h"
#include "ErrorBus.h"
#include "FileContentHelper.h"
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
#include <QDateTime>
#include <QDesktopServices>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QStandardPaths>
#include <QUuid>
#include <QImage>

Q_LOGGING_CATEGORY(lcIpcDispatcher, "gonnect.app.chat.IpcDispatcher")

#define GONNECT_ASSERT_HAS_VERIFICATION                                               \
    if (!hasDeviceVerification()) {                                                   \
        qCCritical(lcIpcDispatcher) << "Received verification content although "      \
                                       "hasDeviceVerification() is false - ignoring"; \
        /*return;*/                                                                   \
    }
#define GONNECT_ASSERT_IS_IN_VERIFICATION_PROCESS                                                \
    if (!m_isInVerificationProcess) {                                                            \
        qCCritical(lcIpcDispatcher) << "Expected to be in verification process, but it is not."; \
        return;                                                                                  \
    }
#define GONNECT_ASSERT_IS_NOT_IN_VERIFICATION_PROCESS                                            \
    if (m_isInVerificationProcess) {                                                             \
        qCCritical(lcIpcDispatcher) << "Expected not to be in verification process, but it is."; \
        return;                                                                                  \
    }

#define GONNECT_ASSERT_VERIFICATION_PROCESS(verificationFlowId)                           \
    if (m_verificationFlowId != verificationFlowId) {                                     \
        qCCritical(lcIpcDispatcher)                                                       \
                << "Received verification content for process" << verificationFlowId      \
                << "but am currently in process" << m_verificationFlowId << "- ignoring"; \
        return;                                                                           \
    }

#define GONNECT_ASSERT(condition, failMessage)                             \
    if (!(condition)) {                                                    \
        qCCritical(lcIpcDispatcher) << "Assertion failed:" << failMessage; \
        return;                                                            \
    }

using namespace de::gonicus::gonnect;
using namespace std::chrono_literals;

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

IChatRoom::UserRoomState IpcDispatcher::userRoomStateConv(
        const de::gonicus::gonnect::UserRoomStateGadget::UserRoomState state)
{
    switch (state) {

    case UserRoomStateGadget::UserRoomState::Unjoined:
        return IChatRoom::UserRoomState::Unjoined;
    case UserRoomStateGadget::UserRoomState::Joined:
        return IChatRoom::UserRoomState::Joined;
    case UserRoomStateGadget::UserRoomState::Invited:
        return IChatRoom::UserRoomState::Invited;
    case UserRoomStateGadget::UserRoomState::Knocked:
        return IChatRoom::UserRoomState::Knocked;
    case UserRoomStateGadget::UserRoomState::Banned:
        return IChatRoom::UserRoomState::Banned;
    }

    qCCritical(lcIpcDispatcher) << "Unknown state:" << state;
    qFatal("Unknown state");
    Q_UNREACHABLE();
}

ChatUser::PresenceState IpcDispatcher::presenceStateConv(
        const de::gonicus::gonnect::PresenceStateGadget::PresenceState state)
{
    switch (state) {

    case PresenceStateGadget::PresenceState::Unknown:
        return ChatUser::PresenceState::Unknown;
    case PresenceStateGadget::PresenceState::Offline:
        return ChatUser::PresenceState::Offline;
    case PresenceStateGadget::PresenceState::Away:
        return ChatUser::PresenceState::Away;
    case PresenceStateGadget::PresenceState::Online:
        return ChatUser::PresenceState::Online;
    }

    qCCritical(lcIpcDispatcher) << "Unknown state:" << state;
    qFatal("Unknown state");
    Q_UNREACHABLE();
}

CrossSigningSecret::CrossSigningMethod IpcDispatcher::crossSigningMethodConv(
        const de::gonicus::gonnect::CrossSigningMethodGadget::CrossSigningMethod method)
{
    switch (method) {

    case CrossSigningMethodGadget::CrossSigningMethod::SasString:
        return CrossSigningSecret::CrossSigningMethod::SasString;
    case CrossSigningMethodGadget::CrossSigningMethod::SasSymbol:
        return CrossSigningSecret::CrossSigningMethod::SasSymbol;
    }

    qCCritical(lcIpcDispatcher) << "Unknown cross signing method:" << method;
    qFatal("Unknown method");
    Q_UNREACHABLE();
}

NotificationSetting::Setting IpcDispatcher::notificationSettingProtoToIpc(
        de::gonicus::gonnect::NotificationSettingGadget::NotificationSetting setting)
{
    switch (setting) {

    case NotificationSettingGadget::NotificationSetting::AllMessages:
        return NotificationSetting::Setting::All;
    case NotificationSettingGadget::NotificationSetting::MentionsAndKeywordsOnly:
        return NotificationSetting::Setting::MentionsAndKeywords;
    case NotificationSettingGadget::NotificationSetting::Mute:
        return NotificationSetting::Setting::Mute;
    }

    return NotificationSetting::Setting::None;
}

::RoomSettings
IpcDispatcher::roomSettingsProtoToIpc(const de::gonicus::gonnect::RoomSettings &protoSettings)
{
    return { protoSettings.hasNotificationSetting()
                     ? notificationSettingProtoToIpc(protoSettings.notificationSetting())
                     : NotificationSetting::Setting::None };
}

NotificationSettingGadget::NotificationSetting
IpcDispatcher::notificationSettingIpcToProto(NotificationSetting::Setting setting)
{
    using Setting = NotificationSettingGadget::NotificationSetting;

    switch (setting) {

    case NotificationSetting::Setting::All:
        return Setting::AllMessages;
    case NotificationSetting::Setting::MentionsAndKeywords:
        return Setting::MentionsAndKeywordsOnly;
    case NotificationSetting::Setting::Mute:
        return Setting::Mute;
    case NotificationSetting::Setting::None:
        // Never happens as "None" is unknown in proto definition
        break;
    }

    // Fallback to make compiler happy
    return Setting::Mute;
}

CrossSigningMethodGadget::CrossSigningMethod
IpcDispatcher::crossSigningMethodReConv(const CrossSigningSecret::CrossSigningMethod method)
{
    switch (method) {

    case CrossSigningSecret::CrossSigningMethod::SasString:
        return CrossSigningMethodGadget::CrossSigningMethod::SasString;
    case CrossSigningSecret::CrossSigningMethod::SasSymbol:
        return CrossSigningMethodGadget::CrossSigningMethod::SasSymbol;
    }

    qCCritical(lcIpcDispatcher) << "Unknown cross signing method:" << method;
    qFatal("Unknown method");
    Q_UNREACHABLE();
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

    auto req = createRequest();
    req->setLoginUsernamePasswordRequest(credReq);
    sendRequest(req);
}

void IpcDispatcher::loginWithSSO(const QString &identityProvider)
{
    Q_ASSERT(identityProvider.isEmpty() || m_identityProviders.contains(identityProvider));

    bool found = false;
    if (identityProvider.isEmpty()) {
        // Empty string means that the default provider must be used
        found = true;

    } else {
        for (const auto &idp : std::as_const(m_identityProviders)) {
            if (idp == identityProvider) {
                found = true;
                break;
            }
        }
    }

    if (!found) {
        qCCritical(lcIpcDispatcher)
                << QString("The identity provider '%1' is unknown. Please use one of these: %2")
                           .arg(identityProvider, m_identityProviders.join(", "));
        return;
    }

    auto req = createRequest();
    LoginSSORequest ssoLoginReq;
    ssoLoginReq.setIdentityProvider(identityProvider);
    req->setLoginSSORequest(ssoLoginReq);
    sendRequest(req);
}

void IpcDispatcher::sendMessage(const QString &roomId, const QString &text,
                                const QString &relatedMessageId)
{
    const auto *chatRoom = chatRoomByRoomId(roomId);
    if (!chatRoom) {
        qCCritical(lcIpcDispatcher) << "Unable to find room with id" << roomId << "- aborting";
        return;
    }

    auto ipcRoom = ipcChatRoomById(roomId);
    if (!ipcRoom) {
        qCCritical(lcIpcDispatcher) << "Unable to find ipc room for room id" << roomId;
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
    auto req = createRequest();

    MessageSendRequest msgReq;
    msgReq.setRoomId(roomId);

    if (!relatedMessageId.isEmpty()) {
        msgReq.setRelatedMessageId(relatedMessageId);
    }

    MessageContentText content;
    content.setContent(text);

    msgReq.setText(content);
    msgReq.setMentionedUserIds(mentionedUserIds);
    req->setMessageSendRequest(msgReq);

    // Create optimistic (pending) message for immediate display
    const auto tag = req->tag();
    const auto tempEventId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    using Flag = ChatMessage::Flag;

    ChatMessage *pendingMsg = nullptr;

    QString nickName;
    if (auto *ownUser = m_users.value(ownUserId(), nullptr)) {
        nickName = ownUser->displayName();
    } else if (!m_configInfo.displayName.isEmpty()) {
        nickName = m_configInfo.displayName;
    } else {
        nickName = ownUserId();
    }
    auto *pendingContent = new ChatMessageContentText(text);
    pendingMsg = new ChatMessage(tempEventId, ownUserId(), nickName, pendingContent,
                                 QDateTime::currentDateTimeUtc(), ipcRoom,
                                 Flag::OwnMessage | Flag::Markdown | Flag::Pending);
    if (!relatedMessageId.isEmpty()) {
        pendingMsg->setRelatedMessageId(relatedMessageId);
    }
    ipcRoom->addExistingMessage(pendingMsg, false, false);

    m_pendingMessages.insert(tag, { roomId, tempEventId });

    if (!sendRequest(req)) {
        m_pendingMessages.remove(tag);

        if (pendingMsg) {
            auto flags = pendingMsg->flags();
            flags.setFlag(Flag::Pending, false);
            flags.setFlag(Flag::Failed, true);
            ipcRoom->setMessageFlags(tempEventId, flags);
        }
    }
}

void IpcDispatcher::sendTypingPing(const QString &roomId)
{
    auto req = createRequest(false);
    RoomTypingRequest typingReq;
    typingReq.setRoomId(roomId);
    req->setRoomTypingRequest(typingReq);
    sendRequest(req);
}

void IpcDispatcher::sendFile(const QString &roomId, const QString &filePath,
                             const QString &originalFileName)
{
    if (!chatRoomByRoomId(roomId)) {
        qCCritical(lcIpcDispatcher) << "Unable to find room with id" << roomId << "- aborting";
        return;
    }

    auto req = createRequest();
    MessageSendRequest msgReq;
    msgReq.setRoomId(roomId);

    MessageContentFile content;
    content.setFilePath(makeRelativeToDataRootPath(filePath));

    if (!originalFileName.isEmpty()) {
        content.setFileName(originalFileName);
    }

    msgReq.setFile(content);
    req->setMessageSendRequest(msgReq);
    sendRequest(req);
}

void IpcDispatcher::respondToInvitation(const QString &roomId, bool acceptInvitation)
{
    auto req = createRequest(false);
    InvitedReply reply;
    reply.setRoomId(roomId);
    reply.setAccepted(acceptInvitation);
    req->setInvitedReply(reply);
    sendRequest(req);
}

void IpcDispatcher::markAsRead(const QString &roomId)
{
    Q_ASSERT(!roomId.isEmpty());

    auto req = createRequest(false);
    RoomMarkAsReadRequest markReq;
    markReq.setRoomId(roomId);
    req->setRoomMarkAsReadRequest(markReq);
    sendRequest(req);
}

void IpcDispatcher::loadMessages(IChatRoom *chatRoom, quint32 n)
{
    Q_CHECK_PTR(chatRoom);

    if (chatRoom->isLoadingMessageHistory()) {
        return;
    }

    chatRoom->setIsLoadingMessageHistory(true);

    auto req = createRequest();
    m_roomListTags.insert(req->tag(), chatRoom->id());
    RoomMessagesRequest msgReq;
    msgReq.setRoomId(chatRoom->id());
    msgReq.setLimit(n);
    msgReq.setOrder(MessagesOrderGadget::MessagesOrder::Backward);

    const auto &existingMessages = chatRoom->chatMessages();
    if (!existingMessages.isEmpty()) {
        msgReq.setFromMessageId(existingMessages.first()->eventId());
    }

    req->setRoomMessagesRequest(msgReq);
    sendRequest(req);
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

    auto req = createRequest();
    m_singleMessageTags.insert(req->tag(), messageId);

    MessageRequest msgReq;
    msgReq.setRoomId(roomId);
    msgReq.setMessageId(messageId);

    req->setMessageRequest(msgReq);
    sendRequest(req);
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

    auto req = createRequest();
    req->setMessageRemoveRequest(removeReq);
    sendRequest(req);
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

    auto req = createRequest();
    req->setMessageChangeRequest(changeReq);
    sendRequest(req);
}

void IpcDispatcher::retrySendMessage(const QString &roomId, const QString &failedMessageId)
{
    auto room = ipcChatRoomById(roomId);
    if (!room) {
        qCCritical(lcIpcDispatcher) << "Unable to find room with id" << roomId
                                    << "for retrying message" << failedMessageId;
        return;
    }

    auto msg = room->chatMessageById(failedMessageId);
    if (!msg) {
        qCCritical(lcIpcDispatcher)
                << "Unable to find message with id" << failedMessageId << "for retrying";
        return;
    }

    auto textContent = qobject_cast<ChatMessageContentText *>(msg->content());
    if (!textContent) {
        qCCritical(lcIpcDispatcher) << "Retry is only supported for text messages, but message"
                                    << failedMessageId << "has no text content";
        return;
    }

    const auto text = textContent->rawText();
    const auto relatedMessageId = msg->relatedMessageId();

    room->removeMessage(failedMessageId);
    sendMessage(roomId, text, relatedMessageId);
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
    const auto configLogLevel = settings.value("logging/level", 2).toUInt();
    QString argLogLevel("off");

    switch (configLogLevel) {
    case 1:
        argLogLevel = "error";
        break;
    case 2:
        argLogLevel = "warn";
        break;
    case 3:
        argLogLevel = "info";
        break;
    case 4:
    case 5:
    case 6:
        argLogLevel = "debug";
        break;
    }
    if (configLogLevel >= 7) {
        argLogLevel = "trace";
    }

    args.append({ "--log-level", argLogLevel });

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

bool IpcDispatcher::sendRequest(RequestContainer *requestContainer, quint32 timeoutSeconds)
{
    QScopedPointer request(requestContainer);
    if (!request) {
        qWarning() << "Cannot send a nullptr request - aborting";
        return false;
    }
    if (!m_ipc.isRunning()) {
        qWarning() << "Local socket is not ready - aborting";
        return false;
    }

    // Check which contents are set
    const QStringList types = contentTypes(*request);

    if (!types.length()) {
        qCCritical(lcIpcDispatcher) << "The request container has no content - sending is aborted";
        return false;
    }

    if (types.length() > 1) {
        qCCritical(lcIpcDispatcher) << QString("The request container should contain exactly one "
                                               "content but has %1: %2 - sending is aborted")
                                               .arg(types.length())
                                               .arg(types.join(", "));
        return false;
    }

    const auto tag = request->tag();
    const QString tagDbg = tag > 0 ? QString("(tag: %1)").arg(tag) : "";
    qCInfo(lcIpcDispatcher).noquote() << "Sending request with content" << types.first() << tagDbg;

    // Handle tag
    if (timeoutSeconds && tag > 0) {

        auto timer = new QTimer(this);
        timer->setSingleShot(true);
        timer->setInterval(timeoutSeconds * 1000);
        timer->callOnTimeout(this, [timer, tag, timeoutSeconds, this]() {
            m_timeoutTimers.remove(tag);

            const auto messageId = m_singleMessageTags.take(tag);
            if (!messageId.isEmpty()) {
                m_failedMessageIds.insert(messageId);
            }

            timer->deleteLater();

            qCCritical(lcIpcDispatcher) << "IPC request with tag" << tag << "has timeout after"
                                        << timeoutSeconds << "seconds";
        });

        m_timeoutTimers.insert(tag, timer);
        timer->start();
    }

    // Serialize and send request
    m_ipc.writeData(request->serialize(&m_protoSerializer));

    return true;
}

void IpcDispatcher::processResponse(
        const de::gonicus::gonnect::ResponseContainer &responseContainer)
{
    const auto &rc = responseContainer;
    const auto tag = rc.tag();

    if (tag > 0) {
        if (auto timer = m_timeoutTimers.value(tag, nullptr)) {
            if (rc.hasMessageReceivedEvent() && !m_singleMessageTags.contains(tag)) {
                m_multipartCount.insert(tag, m_multipartCount.value(tag, 0) + 1);
                timer->start();
            } else {
                m_timeoutTimers.remove(tag);
                timer->stop();
                timer->deleteLater();
            }
        } else if (rc.hasError()) {
            const auto failedMessageId = m_singleMessageTags.take(tag);
            if (!failedMessageId.isEmpty()) {
                m_failedMessageIds.insert(failedMessageId);
            }
            qCCritical(lcIpcDispatcher) << "Received IPC message with tag" << tag
                                        << "although not waiting for it, but it is an error and "
                                           "will be processed anyway.";
        } else {
            qCCritical(lcIpcDispatcher)
                    << "Received IPC message with tag" << tag
                    << "although not waiting for it! The message will be ignored.";
            return;
        }
    }

    const auto typeOfContent = contentType(responseContainer);
    Q_ASSERT(!typeOfContent.isEmpty());

    const QString tagDbg = tag > 0 ? QString("(tag: %1)").arg(tag) : "";
    qCInfo(lcIpcDispatcher).noquote()
            << "Processing response with content" << typeOfContent << tagDbg;

    // Unpack and process payload
    if (rc.hasError()) {
        const auto failedMessageId = m_singleMessageTags.take(tag);
        if (!failedMessageId.isEmpty()) {
            m_failedMessageIds.insert(failedMessageId);
        }

        const auto err = rc.error();
        const auto code = static_cast<quint64>(err.type());
        const QString codeStr = QMetaEnum::fromType<Error::ErrorType>().valueToKey(code);

        if (err.type() == Error::ErrorType::UserNotFound) {
            // This error can happen when a message from a user who does not exist any more is
            // received. The error shall not produce an error message visible to the user, thus it
            // is ignored here.
            return;
        }

        qCCritical(lcIpcDispatcher)
                << QString("Error on IPC chat %1: %2 (%3 (%4))")
                           .arg(m_settingsGroup,
                                err.hasErrorString() ? err.errorString() : "<no error message>",
                                codeStr)
                           .arg(code);

        if (err.hasErrorString()) {
            ErrorBus::instance().addError(tr("An IPC error ocurred (%1, %2 (code %3):\n%4")
                                                  .arg(m_settingsGroup, codeStr)
                                                  .arg(code)
                                                  .arg(err.errorString()));
        } else {
            ErrorBus::instance().addError(tr("An IPC error ocurred (%1, %2 (code %3)")
                                                  .arg(m_settingsGroup, codeStr)
                                                  .arg(code));
        }

        // Mark pending message as failed on error
        if (const auto pendingInfo = m_pendingMessages.take(tag);
            !pendingInfo.tempEventId.isEmpty()) {
            if (auto room = ipcChatRoomById(pendingInfo.roomId)) {
                if (auto chatMsg = room->chatMessageById(pendingInfo.tempEventId)) {
                    room->setMessageFlags(
                            pendingInfo.tempEventId,
                            (chatMsg->flags() & ~ChatMessage::Flags(ChatMessage::Flag::Pending))
                                    | ChatMessage::Flag::Failed);
                }
            }
        }

    } else if (rc.hasMultipartEnd()) {
        const auto count = m_multipartCount.take(tag);
        const auto roomId = m_roomListTags.take(tag);
        if (!roomId.isEmpty()) {
            if (auto room = m_roomLookup.value(roomId, nullptr)) {
                room->setIsLoadingMessageHistory(false);
                if (!count) {
                    room->setIsCompletelyLoaded(true);
                }
            }
        }

    } else if (rc.hasStatusUpdate()) {
        const auto resp = rc.statusUpdate();
        m_connectionState = static_cast<ConnectionState>(static_cast<int>(resp.code()));
        Q_EMIT connectionStateChanged();

        switch (resp.code()) {

        case StatusUpdate_QtProtobufNested::StatusCode::Connected:
            qCInfo(lcIpcDispatcher) << "  Status: Connected";
            break;

        case StatusUpdate_QtProtobufNested::StatusCode::LoggedIn:
            qCInfo(lcIpcDispatcher) << "  Status: LoggedIn";

            if (m_areCapabilitesInitialized) {

                // Initial room list request
                auto req = createRequest();
                RoomListRequest roomListReq;
                roomListReq.setIncludeJoined(true);
                roomListReq.setIncludeUnjoined(true);
                req->setRoomListRequest(roomListReq);
                sendRequest(req);
            }
            break;

        case StatusUpdate_QtProtobufNested::StatusCode::LoggedOut:
            qCInfo(lcIpcDispatcher) << "  Status: LoggedOut";
            break;

        case StatusUpdate_QtProtobufNested::StatusCode::NetworkUnavailable:
            qCInfo(lcIpcDispatcher) << "  Status: NetworkUnavailable";
            break;

        case StatusUpdate_QtProtobufNested::StatusCode::SessionInvalid:
            qCInfo(lcIpcDispatcher) << "  Status: SessionInvalid";
            break;
        }

        if (m_connectionState == ConnectionState::Connected) {
            // Request login flows
            auto req = createRequest();
            req->setLoginFlowsRequest(LoginFlowsRequest());
            sendRequest(req);
        }

    } else if (rc.hasLoginFlowsResponse()) {
        const auto flows = rc.loginFlowsResponse().loginFlows();
        for (const auto flow : flows) {
            switch (flow) {
            case LoginFlowsResponse_QtProtobufNested::LoginFlow::UsernamePassword:
                m_supportedLoginFlows.insert(IpcConfig::LoginFlow::Credentials);
                break;
            case LoginFlowsResponse_QtProtobufNested::LoginFlow::SSO:
                m_supportedLoginFlows.insert(IpcConfig::LoginFlow::SSO);
                break;
            }
        }

        if (m_supportedLoginFlows.contains(IpcConfig::LoginFlow::SSO)) {
            // Request SSO identity providers
            auto req = createRequest();
            req->setIdentityProvidersRequest(IdentityProvidersRequest());
            sendRequest(req);
        } else {
            login();
        }

    } else if (rc.hasIdentityProvidersResponse()) {
        m_identityProviders = rc.identityProvidersResponse().identityProviders();
        login();

    } else if (rc.hasCapabilityEvent()) {
        const auto resp = rc.capabilityEvent();

        m_areCapabilitesInitialized = true;
        m_supportsDirectRooms = resp.directRooms();
        m_supportsGroupRooms = resp.groupRooms();
        m_supportsSubThreads = resp.subThreads();
        m_supportedMimeTypes = resp.mimeTypes();
        m_hasDeviceVerification = resp.clientVerification();

        Q_EMIT capabilitiesInitializedChanged();

        if (m_connectionState == ConnectionState::LoggedIn) {

            // Initial room list request
            auto req = createRequest();
            RoomListRequest roomListReq;
            roomListReq.setIncludeJoined(true);
            roomListReq.setIncludeUnjoined(true);
            req->setRoomListRequest(roomListReq);
            sendRequest(req);
        }

    } else if (rc.hasLoginSSOResponse()) {
        const auto &url = rc.loginSSOResponse().loginUrl();
        const bool ok = QDesktopServices::openUrl(url);
        if (ok) {
            qCInfo(lcIpcDispatcher).noquote() << "Opened browser for authorization at" << url;
        } else {
            qCCritical(lcIpcDispatcher) << "Unable to open browser for authorization at" << url;
        }

    } else if (rc.hasGlobalSettingsEvent()) {
        m_notificationSetting =
                notificationSettingProtoToIpc(rc.globalSettingsEvent().notificationSetting());

    } else if (rc.hasRoomListResponse()) {
        const auto list = rc.roomListResponse().roomList();
        QSet<QString> handledRoomIds;

        // Create or update rooms
        for (const auto &room : list) {
            const auto &roomId = room.roomId();
            auto roomObj = qobject_cast<IpcChatRoom *>(chatRoomByRoomId(roomId));
            const auto roomIdx = m_rooms.indexOf(roomObj);
            handledRoomIds.insert(room.roomId());

            if (roomObj) {
                // Update existing room
                if (room.hasDisplayName() && roomObj->name() != room.displayName()) {
                    roomObj->setName(room.displayName());
                }

                roomObj->setJoinRule(joinRuleGrpcToGonnect(room.joinRule()));
                roomObj->setRoomSettings(roomSettingsProtoToIpc(room.roomSettings()));
            } else {
                // Create new room
                roomObj = addChatRoom(room);
            }

            // Put users in room
            Q_CHECK_PTR(roomObj);
            QHashIterator it(room.userIdList());

            // Setup cache
            QHash<QString, IChatRoom::UserRoomState> *roomCache =
                    m_userRoomStateCache.value(roomId, nullptr);
            if (!roomCache) {
                roomCache = new QHash<QString, IChatRoom::UserRoomState>;
                m_userRoomStateCache.insert(roomId, roomCache);
            }

            while (it.hasNext()) {
                it.next();
                const auto &userId = it.key();
                const auto userRoomState = userRoomStateConv(it.value());

                // Fill information into cache
                roomCache->insert(userId, userRoomState);

                // Set or request user
                auto user = m_users.value(userId, nullptr);
                if (user) {
                    roomObj->addUser(user, userRoomState);
                    if (roomObj->isDirectChat()) {
                        Q_EMIT chatUserPropertiesChanged(user, roomObj, roomIdx);
                    }
                } else {
                    requestUser(userId);
                }
            }
        }

        // Remove obsolete rooms
        QMutableListIterator it(m_rooms);
        while (it.hasNext()) {
            auto room = it.next();
            if (!handledRoomIds.contains(room->id())) {
                removeNotificationsForRoom(room);
                m_roomLookup.remove(room->id());
                Q_EMIT chatRoomRemoved(indexOf(room), room);
                it.remove();
                room->deleteLater();
            }
        }

    } else if (rc.hasRoomCreatedEvent()) {
        addChatRoom(rc.roomCreatedEvent(), QString::number(rc.tag()));

    } else if (rc.hasUserResponse()) {
        const auto &user = rc.userResponse();
        auto &addrBook = AddressBook::instance();

        const auto id = user.userId();
        const auto displayName = user.hasDisplayName() ? user.displayName() : "";

        ChatUser *p = nullptr;
        if (m_users.contains(id)) {
            // Update existing user
            p = m_users.value(id);
            p->setDisplayName(displayName);

        } else {
            // New user object
            const bool hasPresenceState = user.hasStatus();

            QString avatarPath;
            if (user.hasAvatarPath()) {
                avatarPath = makeDataRootPath(user.avatarPath());
            }

            p = new ChatUser(id, displayName, hasPresenceState, avatarPath, this);
            if (hasPresenceState) {
                p->setPresenceState(presenceStateConv(user.status().state()));

                if (auto directRoom = ipcDirectChatRoomForUser(p)) {
                    Q_EMIT chatUserPropertiesChanged(p, directRoom, indexOf(directRoom));
                }
            }
            m_users.insert(id, p);
            m_userList.append(p);

            auto conv = id;
            if (m_useIdConversion) {
                conv.replace(m_idConvRegex, m_configInfo.idConvReplacementString);
            }

            if (!conv.isEmpty()) {
                auto contact = addrBook.lookupByEmail(conv);
                if (contact) {
                    contact->addChatUser(p);
                    m_userContacts.insert(id, contact);
                }
            }

            Q_EMIT userAdded(id, p, m_userList.length() - 1);
        }

        // Update user in chat rooms
        QHashIterator roomIt(m_userRoomStateCache);
        while (roomIt.hasNext()) {
            roomIt.next();

            const auto &roomId = roomIt.key();
            const auto *roomUserMap = roomIt.value();

            if (roomUserMap->contains(id)) {
                const auto userRoomState = roomUserMap->value(id);
                auto *room = m_roomLookup.value(roomId, nullptr);
                if (!room) {
                    qCCritical(lcIpcDispatcher) << "Unable to find room for id" << roomId;
                    continue;
                }

                if (room->chatUserById(p->id())) {
                    room->setUserRoomState(p, userRoomState);
                } else {
                    room->addUser(p, userRoomState);
                    if (room->isDirectChat()) {
                        Q_EMIT chatUserPropertiesChanged(p, room, m_rooms.indexOf(room));
                    }
                }
            }
        }

    } else if (rc.hasUserChangeEvent()) {
        const auto changeEvent = rc.userChangeEvent();
        auto user = m_users.value(changeEvent.userId(), nullptr);
        if (!user) {
            // Ignore unknown user as the full user object must be requested first
            // qCCritical(lcIpcDispatcher)
            //         << "Ignoring UserChangeEvent for user" << changeEvent.userId()
            //         << "because ChatUser object could not be found for that id";
            return;
        }

        bool hasChanged = false;

        if (changeEvent.hasStatus()) {
            hasChanged = true;
            const auto convState = presenceStateConv(changeEvent.status().state());
            user->setHasPresenceState(convState != ChatUser::PresenceState::Unknown);
            user->setPresenceState(convState);
        }

        if (changeEvent.hasDisplayName()) {
            hasChanged = true;
            user->setDisplayName(changeEvent.displayName());
        }

        if (changeEvent.hasAvatarPath()) {
            hasChanged = true;
            user->setAvatarPath(makeDataRootPath(changeEvent.avatarPath()));
        }

        if (hasChanged) {
            auto chatRoom = ipcDirectChatRoomForUser(user);
            qsizetype index = chatRoom ? indexOf(chatRoom) : -1;
            Q_EMIT chatUserPropertiesChanged(user, chatRoom, index);
        } else {
            qCWarning(lcIpcDispatcher)
                    << "Received UserChangeEvent for user" << rc.userChangeEvent().userId()
                    << "but no changes have been made";
        }

    } else if (rc.hasMessageSendResponse()) {

        // Update pending optimistic message with real server event ID
        if (const auto pendingInfo = m_pendingMessages.take(tag);
            !pendingInfo.tempEventId.isEmpty()) {
            if (auto room = ipcChatRoomById(pendingInfo.roomId)) {
                const auto &msgResponse = rc.messageSendResponse();
                room->updateMessageEventId(pendingInfo.tempEventId, msgResponse.messageId());
                if (auto chatMsg = room->chatMessageById(msgResponse.messageId())) {
                    room->setMessageFlags(
                            msgResponse.messageId(),
                            chatMsg->flags() & ~ChatMessage::Flags(ChatMessage::Flag::Pending));
                }
            }
        }

    } else if (rc.hasMessageReceivedEvent()) {

        ChatMessage *chatMessage = nullptr;

        if (const auto pendingInfo = m_pendingMessages.take(tag);
            !pendingInfo.tempEventId.isEmpty()) {

            if (auto room = ipcChatRoomById(pendingInfo.roomId)) {
                const auto &msg = rc.messageReceivedEvent();
                room->updateMessageEventId(pendingInfo.tempEventId, msg.messageId());

                chatMessage = room->chatMessageById(msg.messageId());
            }
        }

        const bool isIndependent = m_singleMessageTags.remove(tag);
        const bool isUnread = !tag;
        const auto chatMessageObj = createOrUpdateReceivedChatMessage(
                rc.messageReceivedEvent(), isUnread, isIndependent, chatMessage);
        if (isUnread && !isIndependent) {
            makeNotificationNewMessage(chatMessageObj);
        }

    } else if (rc.hasMessageChangeEvent()) {
        const auto changeEvent = rc.messageChangeEvent();
        const auto messageId = changeEvent.messageId();

        IpcChatRoom *room = nullptr;
        ChatMessage *message = nullptr;
        qsizetype index = -1;

        for (auto r : std::as_const(m_rooms)) {
            if (r->hasMessage(messageId)) {
                room = r;
                message = r->chatMessageById(messageId);
                index = r->indexOfMessage(message);
                break;
            }
        }

        GONNECT_ASSERT(room && message, "Cannot find room or message for message id " + messageId)

        const auto previousFlags = message->flags();

        const bool hasIsPinnedChanged = changeEvent.hasIsPinned()
                && (static_cast<bool>(message->flags() & ChatMessage::Flag::Pinned)
                    != changeEvent.isPinned());
        if (hasIsPinnedChanged) {
            message->setFlags(message->flags() ^ ChatMessage::Flag::Pinned);
        }

        const bool hasIsEncryptedChanged = changeEvent.hasIsEncrypted()
                && (static_cast<bool>(message->flags() & ChatMessage::Flag::Encrypted)
                    != changeEvent.isEncrypted());
        if (hasIsEncryptedChanged) {
            message->setFlags(message->flags() ^ ChatMessage::Flag::Encrypted);
        }

        // Mentioned users
        bool hasMentionedUsersChanged = false;
        if (changeEvent.hasMentionedUserIdsChanged()) {

            // Collect new users
            QSet<ChatUser *> newUsers;
            const auto userIds = changeEvent.mentionedUserIds();
            for (const auto &userId : userIds) {
                if (auto user = m_users.value(userId, nullptr)) {
                    newUsers.insert(user);
                } else {
                    qCWarning(lcIpcDispatcher) << "Ignoring unknown mentioned user" << userId;
                }
            }

            // Diff with existing users
            QSet<ChatUser *> existing = message->mentionedUsers();
            const auto removed = existing - newUsers;
            const auto added = newUsers - existing;
            message->removeMentionendUsers(removed);
            message->addMentionendUsers(added);

            hasMentionedUsersChanged = !removed.isEmpty() || !added.isEmpty();
        }

        // Content
        bool hasContentChanged = false;

        auto content = message->content();

        if (!content) {
            // Message has no content - create new one
            content = createMessageContent(changeEvent);
            message->setContent(content);
            hasContentChanged = true;

        } else {
            // Content exists - update

            if (const auto messageStateContent =
                        qobject_cast<ChatMessageContentUserStateChange *>(content)) {
                const auto convChange =
                        userStateGrpcToGonnect(changeEvent.membershipChange().change());
                if (changeEvent.hasMembershipChange()
                    && convChange != messageStateContent->state()) {
                    hasContentChanged = true;
                    messageStateContent->setAffectedUserId(
                            changeEvent.membershipChange().affectedUserId());
                    messageStateContent->setSetState(convChange);
                }
            } else if (const auto messageTextContent =
                               qobject_cast<ChatMessageContentText *>(content)) {
                if (changeEvent.hasText()) {
                    hasContentChanged = true;
                    messageTextContent->setText(changeEvent.text().content());
                }
            } else {
                // File

                using FileType = FileContentHelper::FileType;
                const auto fileType = FileContentHelper::fileType(
                        changeEvent.hasFile() ? changeEvent.file().filePath() : "");

                if (const auto messageImageContent =
                            qobject_cast<ChatMessageContentImage *>(content)) {
                    if (fileType == FileType::Image) {
                        const auto convImgPath = makeDataRootPath(changeEvent.file().filePath());
                        if (convImgPath != messageImageContent->imagePath()) {
                            hasContentChanged = true;
                            messageImageContent->setImagePath(convImgPath);
                        }
                    }
                } else if (const auto messageFileContent =
                                   qobject_cast<ChatMessageContentFile *>(content)) {
                    if (changeEvent.hasFile()) {
                        const auto convFilePath = makeDataRootPath(changeEvent.file().filePath());
                        if (convFilePath != messageFileContent->filePath()) {
                            hasContentChanged = true;
                            messageFileContent->setFilePath(convFilePath);
                        }
                        if (changeEvent.file().hasFileName()) {
                            const auto fileName = changeEvent.file().fileName();
                            if (fileName != messageFileContent->fileName()) {
                                hasContentChanged = true;
                                messageFileContent->setFileName(fileName);
                            }
                        }
                    }
                }
            }
        }

        if (hasIsEncryptedChanged || hasIsPinnedChanged) {
            Q_EMIT room->chatMessageFlagsChanged(index, message, previousFlags);
        }
        if (hasContentChanged) {
            Q_EMIT room->chatMessageContentChanged(index, message);
        }
        if (hasMentionedUsersChanged) {
            Q_EMIT room->chatMessageMentionedUsersChanged(index, message);
        }

    } else if (rc.hasMessageRemoveEvent()) {
        const auto messageId = rc.messageRemoveEvent().messageId();
        IpcChatRoom *foundRoom = nullptr;
        for (auto room : std::as_const(m_rooms)) {
            if (room->hasMessage(messageId)) {
                foundRoom = room;
                break;
            }
        }

        if (!foundRoom) {
            qCCritical(lcIpcDispatcher) << "Message with id" << messageId
                                        << "shall be removed, but unable to find that message "
                                           "and its room - ignoring";
            return;
        }

        foundRoom->removeMessage(messageId);

    } else if (rc.hasReactionCreatedEvent()) {
        const auto reaction = rc.reactionCreatedEvent();
        auto room = m_roomLookup.value(reaction.roomId(), nullptr);
        GONNECT_ASSERT(room, "Unable to find room for id " + reaction.roomId())

        auto message = room->chatMessageById(reaction.messageId());
        GONNECT_ASSERT(message, "Unable to find message object for id " + reaction.messageId())

        auto user = m_users.value(reaction.userId(), nullptr);

        if (!user) {
            qCCritical(lcIpcDispatcher)
                    << "Received a reaction for message" << reaction.messageId() << "and user"
                    << reaction.userId() << "but cannot find user - ignoring";
            return;
        }

        message->addReaction(reaction.reaction(), user);

        Q_EMIT room->chatMessageReactionsChanged(room->indexOfMessage(message), message);
        Q_EMIT reactionChanged(reaction.messageId());

    } else if (rc.hasReactionRemovedEvent()) {
        const auto reaction = rc.reactionRemovedEvent();
        auto room = m_roomLookup.value(reaction.roomId(), nullptr);
        GONNECT_ASSERT(room, "Unable to find room for id " + reaction.roomId())

        auto message = room->chatMessageById(reaction.messageId());
        GONNECT_ASSERT(message, "Unable to find message object for id " + reaction.messageId())

        auto user = m_users.value(reaction.userId(), nullptr);

        if (!user) {
            qCCritical(lcIpcDispatcher)
                    << "Received a reaction for message" << reaction.messageId() << "and user"
                    << reaction.userId() << "but cannot find user - ignoring";
            return;
        }

        message->removeReaction(reaction.reaction(), user);

        Q_EMIT room->chatMessageReactionsChanged(room->indexOfMessage(message), message);
        Q_EMIT reactionChanged(reaction.messageId());

    } else if (rc.hasUserSearchResponse()) {
        const auto userList = rc.userSearchResponse().userList();

        QList<ChatUser *> users;
        users.reserve(userList.size());

        for (const auto &user : userList) {
            if (auto userObj = m_users.value(user.userId(), nullptr)) {
                users.append(userObj);
            }
        }

        Q_EMIT chatUserSearchResult(QString::number(rc.tag()), users);

    } else if (rc.hasInvitedEvent()) {
        const auto invitedEvent = rc.invitedEvent();
        const auto displayName =
                invitedEvent.hasRoomDisplayName() ? invitedEvent.roomDisplayName() : "";
        const auto invitationText =
                invitedEvent.hasInvitationText() ? invitedEvent.invitationText() : "";
        Q_EMIT roomInviteReceived(invitedEvent.roomId(), displayName, invitationText);

    } else if (rc.hasPublicRoomListResponse()) {

        auto listResp = rc.publicRoomListResponse();
        if (listResp.hasNextBatch()) {
            m_nextPublicRoomListResponseToken = listResp.nextBatch();
        }

        const auto &rooms = listResp.roomList();

        QList<QSharedPointer<PublicChatRoom>> publicRooms;

        for (const auto &room : rooms) {
            QSharedPointer<PublicChatRoom> p(new PublicChatRoom);
            p->roomId = room.roomId();
            p->numberOfJoinedMembers = room.numJoinedMembers();
            if (room.hasDisplayName()) {
                p->displayName = room.displayName();
            }
            if (room.hasTopic()) {
                p->topic = room.topic();
            }

            using JoinRule = IChatRoom::JoinRule;

            switch (room.joinRule()) {
            case RoomJoinRuleGadget::RoomJoinRule::Invite:
                p->joinRule = JoinRule::Invite;
                break;
            case RoomJoinRuleGadget::RoomJoinRule::Knock:
                p->joinRule = JoinRule::Knock;
                break;
            case RoomJoinRuleGadget::RoomJoinRule::Public:
                p->joinRule = JoinRule::Public;
                break;
            }

            publicRooms.append(p);
        }

        Q_EMIT publicRoomSearchResult(QString::number(tag), publicRooms,
                                      m_nextPublicRoomListResponseToken);

    } else if (rc.hasRoomChangeEvent()) {
        const auto changeEvent = rc.roomChangeEvent();
        const auto &roomId = changeEvent.roomId();
        IpcChatRoom *room = nullptr;

        for (auto r : std::as_const(m_rooms)) {
            if (r->id() == roomId) {
                room = r;
                break;
            }
        }

        if (!room) {
            qCCritical(lcIpcDispatcher) << "Unable to find room object for room id" << roomId
                                        << "aborting further processing";
            return;
        }

        // Room settings
        if (changeEvent.hasRoomSettings()) {
            room->setRoomSettings(roomSettingsProtoToIpc(changeEvent.roomSettings()));
        }

        // Name
        if (changeEvent.hasDisplayName()) {
            room->setName(changeEvent.displayName());
        }

        // Unread count
        if (changeEvent.hasUnreadCount()) {
            if (changeEvent.unreadCount() == 0) {
                removeNotificationsForRoom(room);
            }
            room->setUnreadCount(changeEvent.unreadCount());
        }

        // Is direct room
        if (changeEvent.hasIsDirect() && room->isDirectChat() != changeEvent.isDirect()) {
            room->setIsDirect(changeEvent.isDirect());
        }

        // Favorite
        if (changeEvent.hasIsFavorite() && changeEvent.isFavorite() != room->isFavorite()) {
            room->setIsFavorite(changeEvent.isFavorite());
            updateHasFavoriteRooms();
        }

        // Room permissions
        if (changeEvent.hasPermissions()) {
            room->setPermissions(roomPermissionsGrpcToGonnect(changeEvent.permissions()));
        }

        // Avatar
        if (changeEvent.hasAvatarPath()) {
            room->setAvatarPath(makeDataRootPath(changeEvent.avatarPath()));
        }

        // Update typing users
        if (changeEvent.hasTypingUserIdListChanged()) {
            const auto &typingUserIds = changeEvent.typingUserIdList();
            QList<ChatUser *> l;
            l.reserve(changeEvent.typingUserIdList().length());
            for (const auto &id : typingUserIds) {
                if (id == ownUserId()) {
                    continue;
                }

                auto user = m_users.value(id, nullptr);
                if (user) {
                    l.append(user);
                } else {
                    qCCritical(lcIpcDispatcher) << "Unable to find object for user id" << id;
                }
            }
            room->setTypingUsers(l);
        }

        // Update user states
        if (changeEvent.hasUserIdListChanged()) {

            // Setup cache
            QHash<QString, IChatRoom::UserRoomState> *roomCache =
                    m_userRoomStateCache.value(roomId, nullptr);
            if (!roomCache) {
                roomCache = new QHash<QString, IChatRoom::UserRoomState>;
                m_userRoomStateCache.insert(roomId, roomCache);
            }

            // Set user states
            const auto roomStates = changeEvent.userIdList();
            QHashIterator it(roomStates);
            while (it.hasNext()) {
                it.next();
                const auto &userId = it.key();
                const auto userRoomState = userRoomStateConv(it.value());

                // Fill information into cache
                roomCache->insert(userId, userRoomState);

                auto user = m_users.value(userId, nullptr);
                if (user) {
                    if (room->chatUserById(user->id())) {
                        room->setUserRoomState(user, userRoomState);
                    } else {
                        room->addUser(user, userRoomState);
                    }
                } else {
                    requestUser(userId);
                }
            }
        }

    } else if (rc.hasRoomLeftEvent()) {

        // Notify that user has left the chat room by removing the room from the list
        const auto &leftEvent = rc.roomLeftEvent();
        const auto roomId = leftEvent.roomId();

        QMutableListIterator it(m_rooms);
        while (it.hasNext()) {
            auto room = it.next();
            if (room->id() == roomId) {
                const QString roomName = room->name();
                removeNotificationsForRoom(room);
                m_roomLookup.remove(room->id());
                Q_EMIT chatRoomRemoved(indexOf(room), room);
                it.remove();
                room->deleteLater();

                Q_EMIT chatRoomLeft(roomId, roomName.isEmpty() ? roomId : roomName,
                                    leaveReasonGrpcToGonnect(leftEvent.reason()),
                                    leftEvent.hasMessage() ? leftEvent.message() : "");
                return;
            }
        }

        qCCritical(lcIpcDispatcher) << "IpcDispatcher has been informed that the user left room"
                                    << roomId << "but that room was not found in the model";

    } else if (rc.hasVerificationStatusEvent()) {
        GONNECT_ASSERT_HAS_VERIFICATION

        const auto verificationStatus = rc.verificationStatusEvent();
        setIsDeviceVerified(verificationStatus.isVerified());

        if (m_isRecoveryKeyVerificationAvailable
            != verificationStatus.isRecoveryKeyVerificationAvailable()) {
            m_isRecoveryKeyVerificationAvailable =
                    verificationStatus.isRecoveryKeyVerificationAvailable();
            Q_EMIT isRecoveryKeyVerificationAvailableChanged();
        }
        if (m_isCrossSigningVerificationAvailable != verificationStatus.isCrossSigningAvailable()) {
            m_isCrossSigningVerificationAvailable = verificationStatus.isCrossSigningAvailable();
            Q_EMIT isCrossSigningVerificationAvailableChanged();
        }

    } else if (rc.hasCrossSigningPromptEvent()) {
        GONNECT_ASSERT_HAS_VERIFICATION
        GONNECT_ASSERT_IS_NOT_IN_VERIFICATION_PROCESS

        m_verificationFlowId = rc.crossSigningPromptEvent().verificationFlowId();
        m_verificationTimeoutTimer.start(10min);
        Q_EMIT crossSigningPrompt();

    } else if (rc.hasCrossSigningStartResponse()) {
        GONNECT_ASSERT_HAS_VERIFICATION
        GONNECT_ASSERT_IS_NOT_IN_VERIFICATION_PROCESS

        const auto startResponse = rc.crossSigningStartResponse();
        const auto verificationFlowId = startResponse.verificationFlowId();
        GONNECT_ASSERT(!verificationFlowId.isEmpty(), "verificationFlowId must not be empty")
        m_verificationFlowId = verificationFlowId;

    } else if (rc.hasCrossSigningStartEvent()) {
        GONNECT_ASSERT_HAS_VERIFICATION
        GONNECT_ASSERT_IS_NOT_IN_VERIFICATION_PROCESS
        m_verificationTimeoutTimer.start(2min);

        const auto startEvent = rc.crossSigningStartEvent();

        const auto verificationFlowId = startEvent.verificationFlowId();
        if (!m_verificationFlowId.isEmpty()) {
            GONNECT_ASSERT_VERIFICATION_PROCESS(verificationFlowId);
        }
        m_verificationFlowId = verificationFlowId;

        QList<CrossSigningSecret::CrossSigningMethod> methods;
        const auto &availableMethods = startEvent.availableMethods();
        methods.reserve(availableMethods.size());
        for (const auto &method : availableMethods) {
            methods.append(crossSigningMethodConv(method));
        }

        GONNECT_ASSERT(!methods.isEmpty(),
                       "Received CrossSingingStartEvent with no available methods")

        setIsInVerificationProcess(true);
        Q_EMIT crossSigningMethodSelectRequired(methods);

    } else if (rc.hasCrossSigningMethodSelectedEvent()) {
        GONNECT_ASSERT_HAS_VERIFICATION
        const auto selectedEvent = rc.crossSigningMethodSelectedEvent();
        GONNECT_ASSERT_VERIFICATION_PROCESS(selectedEvent.verificationFlowId())

        m_verificationTimeoutTimer.start(2min);

        CrossSigningSecret secret;
        switch (selectedEvent.selectedMethod()) {

        case CrossSigningMethodGadget::CrossSigningMethod::SasString: {
            secret.setMethod(CrossSigningSecret::CrossSigningMethod::SasString);
            GONNECT_ASSERT(selectedEvent.hasStringCode(),
                           "CrossSigningMethodselectedEvent must have string secret")
            secret.setStringSecret(selectedEvent.stringCode());
            break;
        }
        case CrossSigningMethodGadget::CrossSigningMethod::SasSymbol: {
            secret.setMethod(CrossSigningSecret::CrossSigningMethod::SasSymbol);
            GONNECT_ASSERT(selectedEvent.hasSymbols(),
                           "CrossSigningMethodselectedEvent must have symbols")
            const auto symbols = selectedEvent.symbols().symbols();
            GONNECT_ASSERT(symbols.size(), "Symbols may not be empty")

            QList<CrossSigningSymbol> convSymbols;
            convSymbols.reserve(symbols.size());
            for (const auto &symbol : symbols) {
                convSymbols.append(CrossSigningSymbol(symbol.symbol(), symbol.description()));
            }
            secret.setSymbolSeqence(convSymbols);
            break;
        }
        default:
            qCCritical(lcIpcDispatcher)
                    << "Ignoring unknown cross signing method" << selectedEvent.selectedMethod();
            return;
        }

        Q_EMIT crossSigningAcceptRequired(secret);

    } else if (rc.hasVerificationEndEvent()) {
        GONNECT_ASSERT_HAS_VERIFICATION
        const auto endEvent = rc.verificationEndEvent();
        if (endEvent.hasVerificationFlowId()) {
            GONNECT_ASSERT_VERIFICATION_PROCESS(endEvent.verificationFlowId())
        }
        m_verificationTimeoutTimer.stop();

        GONNECT_ASSERT(endEvent.hasError() || endEvent.hasSuccessful(),
                       "VerificationEndEvent must have either an error or the successful field set")

        if (endEvent.hasError()) {
            Q_EMIT verificationError(endEvent.error());
            setIsDeviceVerified(false);
        } else if (endEvent.hasSuccessful()) {
            setIsDeviceVerified(endEvent.successful());
        }

        m_verificationFlowId.clear();
        setIsInVerificationProcess(false);

    } else {
        qFatal("Received an unimplemented or empty IPC message");
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

ChatMessage *
IpcDispatcher::createOrUpdateReceivedChatMessage(const de::gonicus::gonnect::Message &message,
                                                 bool isUnread, bool isIndependent,
                                                 ChatMessage *chatMessage)
{
    auto room = ipcChatRoomById(message.roomId());

    if (!room) {
        qCCritical(lcIpcDispatcher) << "Received message for unknown room" << message.roomId();
        return nullptr;
    }

    const bool isNew = !chatMessage;

    if (!chatMessage) {
        chatMessage = room->chatMessageById(message.messageId());
    }

    ChatMessage::Flags flags = m_configInfo.userId == message.senderId()
            ? ChatMessage::Flag::OwnMessage
            : ChatMessage::Flag::Unknown;

    flags |= ChatMessage::Flag::Markdown;

    if (isUnread && (flags & ChatMessage::Flag::OwnMessage)) {
        isUnread = false;
    }

    if (message.isPinned()) {
        flags |= ChatMessage::Flag::Pinned;
    }
    if (message.isEncrypted()) {
        flags |= ChatMessage::Flag::Encrypted;
    }

    // Add new message
    const QDateTime dateTime =
            QDateTime::fromMSecsSinceEpoch(message.timestamp(), QTimeZone::utc());

    QObject *content = createMessageContent(message);

    if (chatMessage) {
        room->updateMessageEventId(chatMessage->eventId(), message.messageId());
        chatMessage->setTimestamp(dateTime);
        room->setMessageFlags(chatMessage->eventId(), flags);
        chatMessage->setContent(content);

        if (!isNew) {
            auto idx = room->indexOfMessage(chatMessage);
            room->chatMessageContentChanged(idx, chatMessage);
        }

    } else {
        const auto user = m_users.value(message.senderId(), nullptr);
        const auto userDisplayName =
                (user && !user->displayName().isEmpty()) ? user->displayName() : message.senderId();

        chatMessage = new ChatMessage(message.messageId(), message.senderId(), userDisplayName,
                                      content, dateTime, room, flags);
    }

    if (message.hasRelatedMessageId()) {
        chatMessage->setRelatedMessageId(message.relatedMessageId());
    }

    if (dateTime > room->latestMessageDateTime()) {
        room->setLatestMessageDateTime(dateTime);
    }

    // Mentions
    const auto mentionedUserIds = message.mentionedUserIds();
    for (const QString &userId : mentionedUserIds) {
        if (auto user = m_users.value(userId, nullptr)) {
            chatMessage->addMentionendUser(user);
        } else {
            qCWarning(lcIpcDispatcher) << "Ignoring unknown mentioned user" << userId;
        }
    }

    if (isNew) {
        room->addExistingMessage(chatMessage, isUnread, isIndependent);
    }

    // Reactions
    bool hasReactionAdded = false;
    for (const auto &reaction : message.reactions()) {
        if (!reaction.hasUserId()) {
            qCritical() << "Ignoring message reaction because it has no user id";
            continue;
        }

        auto reactor = m_users.value(reaction.userId(), nullptr);

        if (!reactor) {
            qCritical() << "Ignoring reactions because unable to find user object for id"
                        << reaction.userId();
            continue;
        }

        chatMessage->addReaction(reaction.reaction(), reactor);
        hasReactionAdded = true;
    }

    if (hasReactionAdded) {
        Q_EMIT room->chatMessageReactionsChanged(room->indexOfMessage(chatMessage), chatMessage);
        Q_EMIT reactionChanged(chatMessage->eventId());
    }

    return chatMessage;
}

IpcChatRoom *IpcDispatcher::addChatRoom(const de::gonicus::gonnect::Room &room, const QString &tag)
{
    // Create and sort in new room
    auto roomObj =
            new IpcChatRoom(room.roomId(), room.hasDisplayName() ? room.displayName() : "", this);
    roomObj->setUnreadCount(room.unreadCount());
    roomObj->setJoinRule(joinRuleGrpcToGonnect(room.joinRule()));
    roomObj->setPermissions(roomPermissionsGrpcToGonnect(room.permissions()));
    roomObj->setIsDirect(room.isDirect());
    roomObj->setIsFavorite(room.isFavorite());
    roomObj->setRoomSettings(roomSettingsProtoToIpc(room.roomSettings()));

    if (room.hasLatestMessageTimestamp()) {
        roomObj->setLatestMessageDateTime(
                QDateTime::fromMSecsSinceEpoch(room.latestMessageTimestamp(), QTimeZone::utc()));
    }

    if (room.hasAvatarPath()) {
        roomObj->setAvatarPath(makeDataRootPath(room.avatarPath()));
    }

    m_rooms.append(roomObj);
    m_roomLookup.insert(room.roomId(), roomObj);

    const auto index = m_rooms.length() - 1;
    Q_EMIT chatRoomAdded(index, roomObj, tag);

    connect(roomObj, &IChatRoom::ownUserJoinStateChanged, this,
            &IpcDispatcher::updateUnreadNotificationsCount);

    connect(roomObj, &IChatRoom::notificationCountChanged, this,
            [this](qsizetype) { updateUnreadNotificationsCount(); });

    return roomObj;
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

    // Create title and message
    QString message;
    QString title;
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
        const auto initials = ViewHelper::instance().initials(senderName);
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

RequestContainer *IpcDispatcher::createRequest(bool withTag)
{
    auto container = new RequestContainer;
    container->setTag(withTag ? m_nextFreeTag++ : 0);
    return container;
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
    auto req = createRequest(false);
    InitializationRequest initReq;
    initReq.setBackendUrl(m_configInfo.backendUrl.toString());
    initReq.setDataRootPath(ensureDataFolderExists());
    initReq.setEncryptionSecret(m_configInfo.encryptionSecret);
    initReq.setPersistentStorageSecret(m_configInfo.persistentStorageSecret);
    initReq.setDeviceDisplayName(m_configInfo.displayName);
    req->setInitializationRequest(initReq);
    sendRequest(req);
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
        auto req = createRequest();
        req->setGlobalSettingsRequest(GlobalSettingsRequest());
        sendRequest(req);
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

        auto req = createRequest();
        req->setUserStatusSetOwnRequest(statusReq);
        sendRequest(req);
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

const QString IpcDispatcher::contentType(const de::gonicus::gonnect::ResponseContainer &container)
{
    const auto &meta = container.staticMetaObject;

    for (int i = meta.propertyOffset(); i < meta.propertyCount(); ++i) {
        const auto property = meta.property(i);
        if (QString(property.name()).startsWith("has") && property.isReadable()
            && property.typeName() == QString("bool")) {

            if (property.readOnGadget(&container).toBool()) {
                return QString(property.name()).mid(3);
            }
        }
    }

    return "";
}

IChatRoom::JoinRule IpcDispatcher::joinRuleGrpcToGonnect(
        const de::gonicus::gonnect::RoomJoinRuleGadget::RoomJoinRule grpcRule)
{
    using JoinRule = IChatRoom::JoinRule;

    switch (grpcRule) {

    case RoomJoinRuleGadget::RoomJoinRule::Invite:
        return JoinRule::Invite;
    case RoomJoinRuleGadget::RoomJoinRule::Knock:
        return JoinRule::Knock;
    case RoomJoinRuleGadget::RoomJoinRule::Public:
        return JoinRule::Public;
    }

    qCCritical(lcIpcDispatcher) << "Unknown room join enum value:" << grpcRule;
    return JoinRule::Unknown;
}

RoomJoinRuleGadget::RoomJoinRule
IpcDispatcher::joinRuleGonnectToGrpc(const IChatRoom::JoinRule joinRule)
{
    using Rule = de::gonicus::gonnect::RoomJoinRuleGadget::RoomJoinRule;

    switch (joinRule) {
    case IChatRoom::JoinRule::Invite:
        return Rule::Invite;
    case IChatRoom::JoinRule::Knock:
        return Rule::Knock;
    case IChatRoom::JoinRule::Public:
        return Rule::Public;
    case IChatRoom::JoinRule::Unknown:
        qCCritical(lcIpcDispatcher)
                << "Enum value 'Unknown' cannot be converted - defaulting to 'Invite'";
        return Rule::Invite;
    }

    qCCritical(lcIpcDispatcher) << "Unknown enum value" << joinRule << "- defaulting to 'Invite'";
    return Rule::Invite;
}

IChatRoom::LeaveReason IpcDispatcher::leaveReasonGrpcToGonnect(
        const de::gonicus::gonnect::RoomLeftEvent::RoomLeaveReason leaveReason)
{
    using LeaveReason = IChatRoom::LeaveReason;

    switch (leaveReason) {

    case RoomLeftEvent::RoomLeaveReason::User:
        return LeaveReason::User;
    case RoomLeftEvent::RoomLeaveReason::Kicked:
        return LeaveReason::Kicked;
    case RoomLeftEvent::RoomLeaveReason::Banned:
        return LeaveReason::Banned;
    }

    qCCritical(lcIpcDispatcher) << "Unknown leave reason enum value:" << leaveReason;
    return LeaveReason::Unknown;
}

ChatMessageContentUserStateChange::State IpcDispatcher::userStateGrpcToGonnect(
        de::gonicus::gonnect::MessageContentMembershipChange::MembershipChange change)
{
    using State = ChatMessageContentUserStateChange::State;

    switch (change) {

    case MessageContentMembershipChange_QtProtobufNested::MembershipChange::Joined:
        return State::Joined;
    case MessageContentMembershipChange_QtProtobufNested::MembershipChange::Left:
        return State::Left;
    case MessageContentMembershipChange_QtProtobufNested::MembershipChange::Invited:
        return State::Invited;
    case MessageContentMembershipChange_QtProtobufNested::MembershipChange::Knocked:
        return State::Knocked;
    case MessageContentMembershipChange_QtProtobufNested::MembershipChange::Banned:
        return State::Banned;
    case MessageContentMembershipChange_QtProtobufNested::MembershipChange::Unbanned:
        return State::Unbanned;
    case MessageContentMembershipChange_QtProtobufNested::MembershipChange::Kicked:
        return State::Kicked;
    }

    qCCritical(lcIpcDispatcher) << "Unknown membership shange enum value:" << change;
    return State::Unknown;
}

IChatRoom::Permissions
IpcDispatcher::roomPermissionsGrpcToGonnect(const de::gonicus::gonnect::RoomPermissions permissions)
{
    IChatRoom::Permissions p;

    if (permissions.canEdit()) {
        p |= IChatRoom::Permission::CanEdit;
    }
    if (permissions.canInvite()) {
        p |= IChatRoom::Permission::CanInvite;
    }
    if (permissions.canKick()) {
        p |= IChatRoom::Permission::CanKick;
    }
    if (permissions.canBan()) {
        p |= IChatRoom::Permission::CanBan;
    }

    return p;
}

template <MessageDeliverer T>
QObject *IpcDispatcher::createMessageContent(const T &message) const
{
    QObject *content = nullptr;

    using FileType = FileContentHelper::FileType;
    const auto fileType =
            FileContentHelper::fileType(message.hasFile() ? message.file().filePath() : "");

    if (message.hasMembershipChange()) {
        content = new ChatMessageContentUserStateChange(
                userStateGrpcToGonnect(message.membershipChange().change()),
                message.membershipChange().affectedUserId());
    } else if (fileType == FileType::Image) {
        content = new ChatMessageContentImage(makeDataRootPath(message.file().filePath()));
    } else if (message.hasText()) {
        content = new ChatMessageContentText(message.text().content());
    } else if (fileType == FileType::Audio) {
        content = new ChatMessageContentAudioFile(
                makeDataRootPath(message.file().filePath()),
                message.file().hasFileName() ? message.file().fileName() : "");
    } else if (fileType == FileType::Video) {
        content = new ChatMessageContentVideoFile(
                makeDataRootPath(message.file().filePath()),
                message.file().hasFileName() ? message.file().fileName() : "");
    } else if (message.hasFile()) {
        content = new ChatMessageContentFile(
                makeDataRootPath(message.file().filePath()),
                message.file().hasFileName() ? message.file().fileName() : "");
    }

    return content;
}

template QObject *IpcDispatcher::createMessageContent(const Message &) const;
template QObject *IpcDispatcher::createMessageContent(const MessageChangeEvent &) const;

RoomLeftEvent::RoomLeaveReason
IpcDispatcher::leaveReasonGonnectToGrpc(const IChatRoom::LeaveReason leaveReason)
{
    using Reason = RoomLeftEvent::RoomLeaveReason;

    switch (leaveReason) {

    case IChatRoom::LeaveReason::User:
        return Reason::User;
    case IChatRoom::LeaveReason::Kicked:
        return Reason::Kicked;
    case IChatRoom::LeaveReason::Banned:
        return Reason::Banned;
    case IChatRoom::LeaveReason::Unknown:
        qCCritical(lcIpcDispatcher)
                << "The enum value 'Unknown' cannot be converted - defaulting to 'User'";
        return Reason::User;
    }

    qCCritical(lcIpcDispatcher) << "Unknown enum value" << leaveReason << "- defaulting to 'User'";
    return Reason::User;
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
    auto req = createRequest();
    const auto tag = QString::number(req->tag());

    UserSearchRequest searchReq;
    searchReq.setQuery(searchPhrase);
    searchReq.setLimit(50);
    req->setUserSearchRequest(searchReq);
    sendRequest(req);

    return tag;
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
    // Create request
    auto req = createRequest(false);
    InvitationRequest inv;
    inv.setRoomId(roomId);
    inv.setInvitees(userIds);
    if (!text.isEmpty()) {
        inv.setInvitationText(text);
    }
    req->setInvitationRequest(inv);
    sendRequest(req);
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

    auto req = createRequest();
    const auto tag = QString::number(req->tag());
    req->setPublicRoomListRequest(listReq);
    sendRequest(req);

    return tag;
}

QString IpcDispatcher::requestDirectRoomCreation(const QString &userId, const QString &name,
                                                 const QString &avatarPath)
{
    if (userId.isEmpty()) {
        qCCritical(lcIpcDispatcher) << "Assertion failed: the user id must not be empty";
        return "";
    }

    auto req = createRequest();
    const auto tag = req->tag();
    RoomCreateDirectRequest createReq;
    createReq.setInvitee(userId);
    if (!name.isEmpty()) {
        createReq.setDisplayName(name);
    }
    if (!avatarPath.isEmpty()) {
        createReq.setAvatarPath(makeRelativeToDataRootPath(avatarPath));
    }
    req->setRoomCreateDirectRequest(createReq);
    sendRequest(req);
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

    auto req = createRequest();
    const auto tag = req->tag();
    RoomCreateGroupRequest createReq;
    createReq.setDisplayName(name);
    createReq.setJoinRule(joinRuleGonnectToGrpc(joinRule));
    if (!userIds.isEmpty()) {
        createReq.setInvitees(userIds);
    }
    if (!avatarPath.isEmpty()) {
        createReq.setAvatarPath(makeRelativeToDataRootPath(avatarPath));
    }
    req->setRoomCreateGroupRequest(createReq);
    sendRequest(req);
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

    auto req = createRequest();
    req->setRoomChangeRequest(changeReq);
    sendRequest(req);
}

void IpcDispatcher::requestToggleRoomFavorite(IChatRoom *chatRoom)
{
    GONNECT_ASSERT(chatRoom, "chatRoom pointer must not be nullptr")

    RoomChangeRequest changeReq;
    changeReq.setRoomId(chatRoom->id());
    changeReq.setIsFavorite(!chatRoom->isFavorite());

    auto req = createRequest();
    req->setRoomChangeRequest(changeReq);
    sendRequest(req);
}

void IpcDispatcher::joinRoomRequest(const QString &roomId)
{
    GONNECT_ASSERT(!roomId.isEmpty(), "roomId must not be empty")

    RoomJoinRequest joinReq;
    joinReq.setRoomId(roomId);

    auto req = createRequest(false);
    req->setRoomJoinRequest(joinReq);
    sendRequest(req);
}

void IpcDispatcher::knockRoomRequest(const QString &roomId, const QString &message)
{
    GONNECT_ASSERT(!roomId.isEmpty(), "roomId must not be empty")

    RoomKnockRequest knockReq;
    knockReq.setRoomId(roomId);

    if (!message.isEmpty()) {
        knockReq.setMessage(message);
    }

    auto req = createRequest(false);
    req->setRoomKnockRequest(knockReq);
    sendRequest(req);
}

void IpcDispatcher::requestRoomLeave(const QString &roomId)
{
    GONNECT_ASSERT(!roomId.isEmpty(), "roomId must not be empty")

    RoomLeaveRequest leaveReq;
    leaveReq.setRoomId(roomId);

    auto req = createRequest();
    req->setRoomLeaveRequest(leaveReq);
    sendRequest(req);
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

    auto req = createRequest();
    req->setUserRequest(userReq);
    sendRequest(req);
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

    auto req = createRequest(false);
    req->setCreateReactionRequest(r);
    sendRequest(req);
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

    auto req = createRequest(false);
    req->setRemoveReactionRequest(r);
    sendRequest(req);
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

        Q_UNUSED(roomId)

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

    auto req = createRequest(false);
    RecoveryKeyVerificationRequest recoveryReq;
    recoveryReq.setRecoveryKey(key);
    req->setRecoveryKeyVerificationRequest(recoveryReq);
    sendRequest(req);
}

void IpcDispatcher::acceptVerification()
{
    GONNECT_ASSERT_HAS_VERIFICATION
    GONNECT_ASSERT_IS_IN_VERIFICATION_PROCESS
    GONNECT_ASSERT(!m_verificationFlowId.isEmpty(),
                   "Need a verification flow id for accepting the verification")

    auto req = createRequest(false);
    CrossSigningConfirmRequest acceptReq;
    acceptReq.setVerificationFlowId(m_verificationFlowId);
    req->setCrossSigningConfirmRequest(acceptReq);
    sendRequest(req);
}

void IpcDispatcher::requestVerificationAbort()
{
    GONNECT_ASSERT_HAS_VERIFICATION
    GONNECT_ASSERT_IS_IN_VERIFICATION_PROCESS
    GONNECT_ASSERT(!m_verificationFlowId.isEmpty(),
                   "Need a verification flow id for accepting the verification")

    auto req = createRequest(false);
    VerificationAbortRequest abortReq;
    abortReq.setVerificationFlowId(m_verificationFlowId);
    req->setVerificationAbortRequest(abortReq);
    sendRequest(req);
}

void IpcDispatcher::requestCrossSigningStart()
{
    GONNECT_ASSERT_HAS_VERIFICATION
    GONNECT_ASSERT_IS_NOT_IN_VERIFICATION_PROCESS

    auto req = createRequest(false);
    CrossSigningStartRequest startReq;
    startReq.setSupportedMethods({ CrossSigningMethodGadget::CrossSigningMethod::SasString,
                                   CrossSigningMethodGadget::CrossSigningMethod::SasSymbol });

    if (!m_verificationFlowId.isEmpty()) {
        startReq.setVerificationFlowId(m_verificationFlowId);
    }

    req->setCrossSigningStartRequest(startReq);
    sendRequest(req);
}

void IpcDispatcher::selectCrossSigningMethod(CrossSigningSecret::CrossSigningMethod method)
{
    GONNECT_ASSERT_HAS_VERIFICATION
    GONNECT_ASSERT_IS_IN_VERIFICATION_PROCESS
    GONNECT_ASSERT(!m_verificationFlowId.isEmpty(),
                   "Need a verification flow id for accepting the verification")

    auto req = createRequest(false);
    CrossSigningMethodSelectedRequest selectReq;
    selectReq.setVerificationFlowId(m_verificationFlowId);
    selectReq.setSelectedMethod(crossSigningMethodReConv(method));
    req->setCrossSigningMethodSelectedRequest(selectReq);
    sendRequest(req);
}

#undef GONNECT_ASSERT
#undef GONNECT_ASSERT_VERIFICATION_PROCESS
#undef GONNECT_ASSERT_IS_IN_VERIFICATION_PROCESS
#undef GONNECT_ASSERT_IS_NOT_IN_VERIFICATION_PROCESS
#undef GONNECT_ASSERT_HAS_VERIFICATION
