// Response handling of the IpcDispatcher: processResponse() dispatches every incoming IPC
// message to one of the handle*() methods below. The request side and general infrastructure
// live in IpcDispatcher.cpp.

#include "IpcDispatcher.h"
#include "IpcDispatcherAsserts.h"
#include "IpcProtoConversion.h"
#include "IpcChatRoom.h"
#include "ChatMessage.h"
#include "ChatUser.h"
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

#include <QDateTime>
#include <QDesktopServices>
#include <QMetaEnum>

using namespace de::gonicus::gonnect;
using namespace std::chrono_literals;

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
            markSingleMessageFailed(tag);
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

    const auto typeOfContent = contentType(rc);
    Q_ASSERT(!typeOfContent.isEmpty());

    const QString tagDbg = tag > 0 ? QString("(tag: %1)").arg(tag) : "";
    qCInfo(lcIpcDispatcher).noquote()
            << "Processing response with content" << typeOfContent << tagDbg;

    // Unpack the payload and dispatch it to the matching handler
    if (rc.hasError()) {
        handleError(rc);
    } else if (rc.hasMultipartEnd()) {
        handleMultipartEnd(tag);
    } else if (rc.hasStatusUpdate()) {
        handleStatusUpdate(rc.statusUpdate());
    } else if (rc.hasLoginFlowsResponse()) {
        handleLoginFlowsResponse(rc.loginFlowsResponse());
    } else if (rc.hasIdentityProvidersResponse()) {
        m_identityProviders = rc.identityProvidersResponse().identityProviders();
        login();
    } else if (rc.hasCapabilityEvent()) {
        handleCapabilityEvent(rc.capabilityEvent());
    } else if (rc.hasLoginSSOResponse()) {
        handleLoginSSOResponse(rc.loginSSOResponse());
    } else if (rc.hasGlobalSettingsEvent()) {
        m_notificationSetting =
                notificationSettingProtoToIpc(rc.globalSettingsEvent().notificationSetting());
    } else if (rc.hasRoomListResponse()) {
        handleRoomListResponse(rc.roomListResponse());
    } else if (rc.hasRoomCreatedEvent()) {
        addChatRoom(rc.roomCreatedEvent(), QString::number(tag));
    } else if (rc.hasUserResponse()) {
        handleUserResponse(rc.userResponse());
    } else if (rc.hasUserChangeEvent()) {
        handleUserChangeEvent(rc.userChangeEvent());
    } else if (rc.hasMessageSendResponse()) {
        // Obsolete - branch exists to prevent the qFatal fallback
    } else if (rc.hasMessageReceivedEvent()) {
        handleMessageReceivedEvent(rc.messageReceivedEvent(), tag);
    } else if (rc.hasMessageChangeEvent()) {
        handleMessageChangeEvent(rc.messageChangeEvent());
    } else if (rc.hasMessageRemoveEvent()) {
        handleMessageRemoveEvent(rc.messageRemoveEvent());
    } else if (rc.hasReactionCreatedEvent()) {
        handleReactionEvent(rc.reactionCreatedEvent(), true);
    } else if (rc.hasReactionRemovedEvent()) {
        handleReactionEvent(rc.reactionRemovedEvent(), false);
    } else if (rc.hasUserSearchResponse()) {
        handleUserSearchResponse(rc.userSearchResponse(), tag);
    } else if (rc.hasInvitedEvent()) {
        handleInvitedEvent(rc.invitedEvent());
    } else if (rc.hasPublicRoomListResponse()) {
        handlePublicRoomListResponse(rc.publicRoomListResponse(), tag);
    } else if (rc.hasRoomChangeEvent()) {
        handleRoomChangeEvent(rc.roomChangeEvent());
    } else if (rc.hasRoomLeftEvent()) {
        handleRoomLeftEvent(rc.roomLeftEvent());
    } else if (rc.hasVerificationStatusEvent()) {
        handleVerificationStatusEvent(rc.verificationStatusEvent());
    } else if (rc.hasCrossSigningPromptEvent()) {
        handleCrossSigningPromptEvent(rc.crossSigningPromptEvent());
    } else if (rc.hasCrossSigningStartResponse()) {
        handleCrossSigningStartResponse(rc.crossSigningStartResponse());
    } else if (rc.hasCrossSigningStartEvent()) {
        handleCrossSigningStartEvent(rc.crossSigningStartEvent());
    } else if (rc.hasCrossSigningMethodSelectedEvent()) {
        handleCrossSigningMethodSelectedEvent(rc.crossSigningMethodSelectedEvent());
    } else if (rc.hasVerificationEndEvent()) {
        handleVerificationEndEvent(rc.verificationEndEvent());
    } else {
        qFatal("Received an unimplemented or empty IPC message");
    }
}

void IpcDispatcher::handleError(const de::gonicus::gonnect::ResponseContainer &responseContainer)
{
    markSingleMessageFailed(responseContainer.tag());

    const auto err = responseContainer.error();
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
}

void IpcDispatcher::handleMultipartEnd(quint64 tag)
{
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
}

void IpcDispatcher::handleStatusUpdate(const de::gonicus::gonnect::StatusUpdate &statusUpdate)
{
    m_connectionState = static_cast<ConnectionState>(static_cast<int>(statusUpdate.code()));
    Q_EMIT connectionStateChanged();

    switch (statusUpdate.code()) {

    case StatusUpdate_QtProtobufNested::StatusCode::Disconnected:
        qCInfo(lcIpcDispatcher) << "  Status: Disconnected";
        break;

    case StatusUpdate_QtProtobufNested::StatusCode::Connected:
        qCInfo(lcIpcDispatcher) << "  Status: Connected";
        break;

    case StatusUpdate_QtProtobufNested::StatusCode::LoggedIn:
        qCInfo(lcIpcDispatcher) << "  Status: LoggedIn";

        if (m_areCapabilitesInitialized) {
            requestRoomList();
        }
        break;
    }

    if (m_connectionState == ConnectionState::Connected) {
        // Request login flows
        sendRequest([](RequestContainer &req) { req.setLoginFlowsRequest(LoginFlowsRequest()); });
    }
}

void IpcDispatcher::handleLoginFlowsResponse(
        const de::gonicus::gonnect::LoginFlowsResponse &response)
{
    const auto flows = response.loginFlows();
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
        sendRequest([](RequestContainer &req) {
            req.setIdentityProvidersRequest(IdentityProvidersRequest());
        });
    } else {
        login();
    }
}

void IpcDispatcher::handleCapabilityEvent(const de::gonicus::gonnect::CapabilityEvent &event)
{
    m_areCapabilitesInitialized = true;
    m_supportsDirectRooms = event.directRooms();
    m_supportsGroupRooms = event.groupRooms();
    m_supportsSubThreads = event.subThreads();
    m_supportedMimeTypes = event.mimeTypes();
    m_hasDeviceVerification = event.clientVerification();

    Q_EMIT capabilitiesInitializedChanged();

    if (m_connectionState == ConnectionState::LoggedIn) {
        requestRoomList();
    }
}

void IpcDispatcher::handleLoginSSOResponse(const de::gonicus::gonnect::LoginSSOResponse &response)
{
    const auto &url = response.loginUrl();
    const bool ok = QDesktopServices::openUrl(url);
    if (ok) {
        qCInfo(lcIpcDispatcher).noquote() << "Opened browser for authorization at" << url;
    } else {
        qCCritical(lcIpcDispatcher) << "Unable to open browser for authorization at" << url;
    }
}

void IpcDispatcher::handleRoomListResponse(const de::gonicus::gonnect::RoomListResponse &response)
{
    const auto list = response.roomList();
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

        auto roomCache = ensureUserRoomStateCache(roomId);

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
    const auto roomsSnapshot = m_rooms;
    for (auto *room : roomsSnapshot) {
        if (!handledRoomIds.contains(room->id())) {
            removeRoom(room);
        }
    }
}

void IpcDispatcher::handleUserResponse(const de::gonicus::gonnect::User &user)
{
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

            if (room->isUserMemberOfRoom(p->id())) {
                room->setUserRoomState(p, userRoomState);
            } else {
                room->addUser(p, userRoomState);
                if (room->isDirectChat()) {
                    Q_EMIT chatUserPropertiesChanged(p, room, m_rooms.indexOf(room));
                }
            }
        }
    }
}

void IpcDispatcher::handleUserChangeEvent(const de::gonicus::gonnect::UserChangeEvent &changeEvent)
{
    auto user = m_users.value(changeEvent.userId(), nullptr);
    if (!user) {
        // Ignore unknown user as the full user object must be requested first
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
        qCWarning(lcIpcDispatcher) << "Received UserChangeEvent for user" << changeEvent.userId()
                                   << "but no changes have been made";
    }
}

void IpcDispatcher::handleMessageReceivedEvent(const de::gonicus::gonnect::Message &message,
                                               quint64 tag)
{
    const bool isIndependent = m_singleMessageTags.remove(tag);
    const bool isUnread = !tag;
    const auto chatMessageObj = addReceivedChatMessage(message, isUnread, isIndependent);
    if (isUnread && !isIndependent) {
        makeNotificationNewMessage(chatMessageObj);
    }
}

void IpcDispatcher::handleMessageChangeEvent(
        const de::gonicus::gonnect::MessageChangeEvent &changeEvent)
{
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

    if (const auto messageStateContent =
                qobject_cast<ChatMessageContentUserStateChange *>(message->content())) {
        const auto convChange = userStateGrpcToGonnect(changeEvent.membershipChange().change());
        if (changeEvent.hasMembershipChange() && convChange != messageStateContent->state()) {
            hasContentChanged = true;
            messageStateContent->setAffectedUserId(changeEvent.membershipChange().affectedUserId());
            messageStateContent->setSetState(convChange);
        }
    } else if (const auto messageTextContent =
                       qobject_cast<ChatMessageContentText *>(message->content())) {
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
                    qobject_cast<ChatMessageContentImage *>(message->content())) {
            if (fileType == FileType::Image) {
                const auto convImgPath = makeDataRootPath(changeEvent.file().filePath());
                if (convImgPath != messageImageContent->imagePath()) {
                    hasContentChanged = true;
                    messageImageContent->setImagePath(convImgPath);
                }
            }
        } else if (const auto messageFileContent =
                           qobject_cast<ChatMessageContentFile *>(message->content())) {
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

    if (hasIsEncryptedChanged || hasIsPinnedChanged) {
        Q_EMIT room->chatMessageFlagsChanged(index, message);
    }
    if (hasContentChanged) {
        Q_EMIT room->chatMessageContentChanged(index, message);
    }
    if (hasMentionedUsersChanged) {
        Q_EMIT room->chatMessageMentionedUsersChanged(index, message);
    }
}

void IpcDispatcher::handleMessageRemoveEvent(
        const de::gonicus::gonnect::MessageRemoveEvent &removeEvent)
{
    const auto messageId = removeEvent.messageId();
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
}

void IpcDispatcher::handleReactionEvent(const de::gonicus::gonnect::Reaction &reaction, bool added)
{
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

    if (added) {
        message->addReaction(reaction.reaction(), user);
    } else {
        message->removeReaction(reaction.reaction(), user);
    }

    Q_EMIT room->chatMessageReactionsChanged(room->indexOfMessage(message), message);
    Q_EMIT reactionChanged(reaction.messageId());
}

void IpcDispatcher::handleUserSearchResponse(
        const de::gonicus::gonnect::UserSearchResponse &response, quint64 tag)
{
    const auto userList = response.userList();

    QList<ChatUser *> users;
    users.reserve(userList.size());

    for (const auto &user : userList) {
        if (auto userObj = m_users.value(user.userId(), nullptr)) {
            users.append(userObj);
        }
    }

    Q_EMIT chatUserSearchResult(QString::number(tag), users);
}

void IpcDispatcher::handleInvitedEvent(const de::gonicus::gonnect::InvitedEvent &invitedEvent)
{
    const auto displayName =
            invitedEvent.hasRoomDisplayName() ? invitedEvent.roomDisplayName() : "";
    const auto invitationText =
            invitedEvent.hasInvitationText() ? invitedEvent.invitationText() : "";
    Q_EMIT roomInviteReceived(invitedEvent.roomId(), displayName, invitationText);
}

void IpcDispatcher::handlePublicRoomListResponse(
        const de::gonicus::gonnect::PublicRoomListResponse &response, quint64 tag)
{
    if (response.hasNextBatch()) {
        m_nextPublicRoomListResponseToken = response.nextBatch();
    }

    const auto &rooms = response.roomList();

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
        p->joinRule = joinRuleGrpcToGonnect(room.joinRule());

        publicRooms.append(p);
    }

    Q_EMIT publicRoomSearchResult(QString::number(tag), publicRooms,
                                  m_nextPublicRoomListResponseToken);
}

void IpcDispatcher::handleRoomChangeEvent(
        const de::gonicus::gonnect::RoomChangeEvent &changeEvent)
{
    const auto &roomId = changeEvent.roomId();
    auto *room = m_roomLookup.value(roomId, nullptr);

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

        auto roomCache = ensureUserRoomStateCache(roomId);

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

                if (room->chatUserRoomState(user) == IChatRoom::UserRoomState::Unjoined) {
                    room->addUser(user, userRoomState);
                } else {
                    room->setUserRoomState(user, userRoomState);
                }
            } else {
                requestUser(userId);
            }
        }
    }
}

void IpcDispatcher::handleRoomLeftEvent(const de::gonicus::gonnect::RoomLeftEvent &leftEvent)
{
    // Notify that user has left the chat room by removing the room from the list
    const auto roomId = leftEvent.roomId();

    auto *room = m_roomLookup.value(roomId, nullptr);
    if (!room) {
        qCCritical(lcIpcDispatcher) << "IpcDispatcher has been informed that the user left room"
                                    << roomId << "but that room was not found in the model";
        return;
    }

    const QString roomName = room->name();
    removeRoom(room);

    Q_EMIT chatRoomLeft(roomId, roomName.isEmpty() ? roomId : roomName,
                        leaveReasonGrpcToGonnect(leftEvent.reason()),
                        leftEvent.hasMessage() ? leftEvent.message() : "");
}

void IpcDispatcher::handleVerificationStatusEvent(
        const de::gonicus::gonnect::VerificationStatusEvent &statusEvent)
{
    GONNECT_ASSERT_HAS_VERIFICATION

    setIsDeviceVerified(statusEvent.isVerified());

    if (m_isRecoveryKeyVerificationAvailable
        != statusEvent.isRecoveryKeyVerificationAvailable()) {
        m_isRecoveryKeyVerificationAvailable = statusEvent.isRecoveryKeyVerificationAvailable();
        Q_EMIT isRecoveryKeyVerificationAvailableChanged();
    }
    if (m_isCrossSigningVerificationAvailable != statusEvent.isCrossSigningAvailable()) {
        m_isCrossSigningVerificationAvailable = statusEvent.isCrossSigningAvailable();
        Q_EMIT isCrossSigningVerificationAvailableChanged();
    }
}

void IpcDispatcher::handleCrossSigningPromptEvent(
        const de::gonicus::gonnect::CrossSigningPromptEvent &promptEvent)
{
    GONNECT_ASSERT_HAS_VERIFICATION
    GONNECT_ASSERT_IS_NOT_IN_VERIFICATION_PROCESS

    m_verificationFlowId = promptEvent.verificationFlowId();
    m_verificationTimeoutTimer.start(10min);
    Q_EMIT crossSigningPrompt();
}

void IpcDispatcher::handleCrossSigningStartResponse(
        const de::gonicus::gonnect::CrossSigningStartResponse &startResponse)
{
    GONNECT_ASSERT_HAS_VERIFICATION
    GONNECT_ASSERT_IS_NOT_IN_VERIFICATION_PROCESS

    const auto verificationFlowId = startResponse.verificationFlowId();
    GONNECT_ASSERT(!verificationFlowId.isEmpty(), "verificationFlowId must not be empty")
    m_verificationFlowId = verificationFlowId;
}

void IpcDispatcher::handleCrossSigningStartEvent(
        const de::gonicus::gonnect::CrossSigningStartEvent &startEvent)
{
    GONNECT_ASSERT_HAS_VERIFICATION
    GONNECT_ASSERT_IS_NOT_IN_VERIFICATION_PROCESS
    m_verificationTimeoutTimer.start(2min);

    const auto verificationFlowId = startEvent.verificationFlowId();
    if (!m_verificationFlowId.isEmpty()) {
        GONNECT_ASSERT_VERIFICATION_PROCESS(verificationFlowId);
    }
    m_verificationFlowId = verificationFlowId;

    QList<CrossSigningSecret::CrossSigningMethod> methods;
    const auto &availableMethods = startEvent.availableMethods();
    for (const auto &method : availableMethods) {
        methods.append(crossSigningMethodConv(method));
    }

    GONNECT_ASSERT(!methods.isEmpty(), "Received CrossSingingStartEvent with no available methods")

    setIsInVerificationProcess(true);
    Q_EMIT crossSigningMethodSelectRequired(methods);
}

void IpcDispatcher::handleCrossSigningMethodSelectedEvent(
        const de::gonicus::gonnect::CrossSigningMethodSelectedEvent &selectedEvent)
{
    GONNECT_ASSERT_HAS_VERIFICATION
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
}

void IpcDispatcher::handleVerificationEndEvent(
        const de::gonicus::gonnect::VerificationEndEvent &endEvent)
{
    GONNECT_ASSERT_HAS_VERIFICATION

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
}

void IpcDispatcher::requestRoomList()
{
    RoomListRequest roomListReq;
    roomListReq.setIncludeJoined(true);
    roomListReq.setIncludeUnjoined(true);
    sendRequest([&](RequestContainer &req) { req.setRoomListRequest(roomListReq); });
}

void IpcDispatcher::removeRoom(IpcChatRoom *room)
{
    removeNotificationsForRoom(room);
    m_roomLookup.remove(room->id());

    const auto index = m_rooms.indexOf(room);
    Q_EMIT chatRoomRemoved(index, room);
    m_rooms.removeAt(index);

    room->deleteLater();
}

QHash<QString, IChatRoom::UserRoomState> *
IpcDispatcher::ensureUserRoomStateCache(const QString &roomId)
{
    auto *roomCache = m_userRoomStateCache.value(roomId, nullptr);
    if (!roomCache) {
        roomCache = new QHash<QString, IChatRoom::UserRoomState>;
        m_userRoomStateCache.insert(roomId, roomCache);
    }
    return roomCache;
}

ChatMessage *IpcDispatcher::addReceivedChatMessage(const de::gonicus::gonnect::Message &message,
                                                   bool isUnread, bool isIndependent)
{
    auto room = ipcChatRoomById(message.roomId());

    if (!room) {
        qCCritical(lcIpcDispatcher) << "Received message for unknown room" << message.roomId();
        return nullptr;
    }

    if (auto *existing = room->chatMessageById(message.messageId())) {
        if (!isIndependent && !room->hasMessage(existing)) {
            room->addExistingMessage(existing, false, false);
        }
        return existing;
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

    const auto user = m_users.value(message.senderId(), nullptr);
    const auto userDisplayName =
            (user && !user->displayName().isEmpty()) ? user->displayName() : message.senderId();

    // Add new message
    const QDateTime dateTime =
            QDateTime::fromMSecsSinceEpoch(message.timestamp(), QTimeZone::utc());

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

    auto newChatMessage = new ChatMessage(message.messageId(), message.senderId(), userDisplayName,
                                          content, dateTime, room, flags);

    if (message.hasRelatedMessageId()) {
        newChatMessage->setRelatedMessageId(message.relatedMessageId());
    }

    if (dateTime > room->latestMessageDateTime()) {
        room->setLatestMessageDateTime(dateTime);
    }

    // Mentions
    const auto mentionedUserIds = message.mentionedUserIds();
    for (const QString &userId : mentionedUserIds) {
        if (auto user = m_users.value(userId, nullptr)) {
            newChatMessage->addMentionendUser(user);
        } else {
            qCWarning(lcIpcDispatcher) << "Ignoring unknown mentioned user" << userId;
        }
    }

    room->addExistingMessage(newChatMessage, isUnread, isIndependent);

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

        newChatMessage->addReaction(reaction.reaction(), reactor);
        hasReactionAdded = true;
    }

    if (hasReactionAdded) {
        Q_EMIT room->chatMessageReactionsChanged(room->indexOfMessage(newChatMessage),
                                                 newChatMessage);
        Q_EMIT reactionChanged(newChatMessage->eventId());
    }

    return newChatMessage;
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
