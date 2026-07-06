#pragma once

#include "IChatProvider.h"
#include "IpcConfig.h"
#include "IpcInterface.h"
#include "NotificationSetting.h"
#include "chat.qpb.h"
#include "IChatRoom.h"
#include "ChatUser.h"

#include <QLocalServer>
#include <QLocalSocket>
#include <QPointer>
#include <QProcess>
#include <QProtobufSerializer>
#include <QTimer>
#include <QtEndian>
#include <QUrl>
#include <QRegularExpression>

#include <utility>

class IpcChatRoom;
class ChatMessage;
class Contact;
class Notification;

#define GONNECT_IPC_TIMEOUT_SECS 29

class IpcDispatcher : public IChatProvider
{
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

public:
    /// Sanity check for the given IpcConfig. Returns whether it is semantically valid, but without
    /// checking the real connectivity.
    static bool checkConfig(const IpcConfig &config);

    /// Whether the config requires a secret of sorts (e.g. password) in order to be used.
    static bool requiresSecret(const IpcConfig &config);

    enum class ConnectionState {
        Disconnected =
                static_cast<int>(de::gonicus::gonnect::StatusUpdate::StatusCode::Disconnected),
        Connected = static_cast<int>(de::gonicus::gonnect::StatusUpdate::StatusCode::Connected),
        LoggedIn = static_cast<int>(de::gonicus::gonnect::StatusUpdate::StatusCode::LoggedIn),
    };
    Q_ENUM(ConnectionState)

    explicit IpcDispatcher(const QString &settingsGroup, const IpcConfig &configInfo,
                           QObject *parent = nullptr);
    ~IpcDispatcher();

    virtual Capabilities capabilities() const override;

    virtual QString ownUserId() const override { return m_configInfo.userId; }

    /// Make the headless client try to login to the backend with the given credentials.
    void loginWithCredentials(const QString &userId, const QString &secret);

    /// Start SSO login process with the given identity provider, which must be one of those in
    /// identityProviders(). Further interactive parts of the flow will be triggered by signals.
    void loginWithSSO(const QString &identityProvider);

    /// List of identity providers as strings as returned by the headless client. This list will
    /// only be filled if the backend supports SSO and the list has successfully been received.
    const QStringList &identityProviders() const { return m_identityProviders; }

    /// The IpcDispatcher is initialized when the sub process has been started and is connected via
    /// local socket. That does not mean that the client is logged in.
    bool isInitialized() const { return m_isInitialized; }

    /// The current connection state of the headless client to the server.
    IpcDispatcher::ConnectionState connectionState() const { return m_connectionState; }

    /// After login, the client awaits the capabilities of the server to be received. This flag
    /// indicates whether that has happend already.
    bool areCapabilitesInitialized() const { return m_areCapabilitesInitialized; }

    /// Send a text message in the specified room.
    void sendMessage(const QString &roomId, const QString &text,
                     const QString &relatedMessageId = "");

    void sendTypingPing(const QString &roomId);

    /// Send a message in the specified room with the file as an attachment. The file must
    /// have already been uploaded.
    void sendFile(const QString &roomId, const QString &filePath,
                  const QString &originalFileName = "");

    /// Call to accept or reject a preceeding invitation to a room.
    virtual void respondToInvitation(const QString &roomId, bool acceptInvitation) override;

    /// Mark the given room as read such there are no unread notifications or messages afterwards.
    void markAsRead(const QString &roomId);

    /// Initial load of room messages.
    void loadMessages(IChatRoom *chatRoom);

    /// Load a single messe. It will be available in lookup, but not in the indexed message list.
    void loadSingleMessage(const QString &roomId, const QString &messageId);

    // IChatProvider interface
    virtual qsizetype chatRoomsCount() override;
    virtual IChatRoom *chatRoomByIndex(qsizetype index) override;
    virtual bool hasFavoriteRooms() const override;
    virtual qsizetype indexOf(IChatRoom *chatRoom) const override;
    virtual IChatRoom *chatRoomByRoomId(const QString &roomId) const override;
    virtual QString displayName() override { return m_configInfo.displayName; }
    virtual QString chatRoomIdForUser(const ChatUser *user) const override;
    virtual QString chatRoomIdForUser(const QString &userId) const override;
    virtual QString searchChatUser(const QString &searchPhrase) override;
    virtual QList<const ChatUser *> users() const override;
    virtual ChatUser *userById(const QString &userId) const override;
    virtual void inviteUsers(const QString &roomId, const QList<QString> &userIds,
                             const QString &text = "") override;
    virtual QString searchPublicRoomRequest(const QString &searchText, const QString &offsetToken,
                                            quint32 limit) override;
    virtual QString requestDirectRoomCreation(const QString &userId, const QString &name,
                                              const QString &avatarPath = "") override;
    virtual QString requestGroupRoomCreation(const QStringList &userIds,
                                             const IChatRoom::JoinRule joinRule,
                                             const QString &name,
                                             const QString &avatarPath = "") override;
    virtual void requestRoomChange(IChatRoom *chatRoom, const QString &name,
                                   IChatRoom::JoinRule joinRule,
                                   const QString &avatarPath) override;
    virtual void requestToggleRoomFavorite(IChatRoom *chatRoom) override;
    virtual void joinRoomRequest(const QString &roomId) override;
    virtual void knockRoomRequest(const QString &roomId, const QString &message = "") override;
    virtual void requestRoomLeave(const QString &roomId) override;
    virtual void requestUser(const QString &userId) override;

    virtual void requestRemoveMessage(const QString &roomId, const QString &messageId) override;
    virtual void requestEditMessage(const QString &roomId, const QString &messageId,
                                    const QString &newContent) override;

    virtual void addReaction(const QString &roomId, const QString &messageId,
                             const QString &reaction) override;
    virtual void retractReaction(const QString &roomId, const QString &messageId,
                                 const QString &reaction) override;
    virtual void toggleReaction(const QString &roomId, const QString &messageId,
                                const QString &reaction) override;

    virtual QString uploadFile(const QString &filePath) override;
    virtual void uploadImageFromClipboard(const QString &roomId) override;

    virtual bool hasDeviceVerification() const override { return m_hasDeviceVerification; }
    virtual bool isDeviceVerified() const override { return m_isDeviceVerified; }
    virtual bool isRecoveryKeyVerificationAvailable() const override
    {
        return m_isRecoveryKeyVerificationAvailable;
    }
    virtual bool isCrossSigningVerificationAvailable() const override
    {
        return m_isCrossSigningVerificationAvailable;
    }
    virtual bool isInVerificationProcess() const override { return m_isInVerificationProcess; }
    virtual void requestRecoveryKeyVerification(const QString &key) override;
    virtual void acceptVerification() override;
    virtual void requestVerificationAbort() override;
    virtual void requestCrossSigningStart() override;
    virtual void selectCrossSigningMethod(CrossSigningSecret::CrossSigningMethod method) override;

private Q_SLOTS:
    void onBlobReceived(const QByteArray data);
    void updateConnected();
    void updateHasFavoriteRooms();
    void sendInitialInitializationRequest();
    void onLoggedInChanged();
    void forwardOwnPresenceState();
    void updateUnreadNotificationsCount();
    void updateUnreadNotificationsCountImpl();

private:
    /// Retrieves a list of types that are set in this request container.
    static QStringList contentTypes(const de::gonicus::gonnect::RequestContainer &container);

    /// The content type that is set in this response container.
    static const QString contentType(const de::gonicus::gonnect::ResponseContainer &container);

    /// Setup and start sub process.
    void init();

    /// Try to login with the information given in the IpcConfig struct. Should only be called when
    /// the headless client is ready to process the login, i.e. all forgoing information (like IdP)
    /// has been received.
    void login();

    /// Create a request container, let fill set its payload, then serialize and send it. If
    /// withTag is true, a unique tag is generated - an answer to the request must then be received
    /// within timeoutSeconds, or a timeout occurs. Otherwise the container carries the zero id for
    /// fire and forget requests. Returns the tag of the sent request (0 for fire and forget).
    template <typename FillFunc>
    quint64 sendRequest(FillFunc &&fill, bool withTag = true,
                        quint32 timeoutSeconds = GONNECT_IPC_TIMEOUT_SECS)
    {
        de::gonicus::gonnect::RequestContainer container;
        container.setTag(withTag ? m_nextFreeTag++ : 0);
        fill(container);
        sendRequestContainer(container, timeoutSeconds);
        return container.tag();
    }

    /// Serialize and send the request container after sanity checks; starts the timeout timer for
    /// tagged requests. Use sendRequest() instead of calling this directly.
    void sendRequestContainer(de::gonicus::gonnect::RequestContainer &container,
                              quint32 timeoutSeconds);

    /// Dispatch the response container and its content payload to the matching handler below.
    void processResponse(const de::gonicus::gonnect::ResponseContainer &responseContainer);

    // Response handlers (implemented in IpcDispatcherResponses.cpp)
    void handleError(const de::gonicus::gonnect::ResponseContainer &responseContainer);
    void handleMultipartEnd(quint64 tag);
    void handleStatusUpdate(const de::gonicus::gonnect::StatusUpdate &statusUpdate);
    void handleLoginFlowsResponse(const de::gonicus::gonnect::LoginFlowsResponse &response);
    void handleCapabilityEvent(const de::gonicus::gonnect::CapabilityEvent &event);
    void handleLoginSSOResponse(const de::gonicus::gonnect::LoginSSOResponse &response);
    void handleRoomListResponse(const de::gonicus::gonnect::RoomListResponse &response);
    void handleUserResponse(const de::gonicus::gonnect::User &user);
    void handleUserChangeEvent(const de::gonicus::gonnect::UserChangeEvent &changeEvent);
    void handleMessageReceivedEvent(const de::gonicus::gonnect::Message &message, quint64 tag);
    void handleMessageChangeEvent(const de::gonicus::gonnect::MessageChangeEvent &changeEvent);
    void handleMessageRemoveEvent(const de::gonicus::gonnect::MessageRemoveEvent &removeEvent);
    void handleReactionEvent(const de::gonicus::gonnect::Reaction &reaction, bool added);
    void handleUserSearchResponse(const de::gonicus::gonnect::UserSearchResponse &response,
                                  quint64 tag);
    void handleInvitedEvent(const de::gonicus::gonnect::InvitedEvent &invitedEvent);
    void handlePublicRoomListResponse(const de::gonicus::gonnect::PublicRoomListResponse &response,
                                      quint64 tag);
    void handleRoomChangeEvent(const de::gonicus::gonnect::RoomChangeEvent &changeEvent);
    void handleRoomLeftEvent(const de::gonicus::gonnect::RoomLeftEvent &leftEvent);
    void
    handleVerificationStatusEvent(const de::gonicus::gonnect::VerificationStatusEvent &statusEvent);
    void
    handleCrossSigningPromptEvent(const de::gonicus::gonnect::CrossSigningPromptEvent &promptEvent);
    void handleCrossSigningStartResponse(
            const de::gonicus::gonnect::CrossSigningStartResponse &startResponse);
    void
    handleCrossSigningStartEvent(const de::gonicus::gonnect::CrossSigningStartEvent &startEvent);
    void handleCrossSigningMethodSelectedEvent(
            const de::gonicus::gonnect::CrossSigningMethodSelectedEvent &selectedEvent);
    void handleVerificationEndEvent(const de::gonicus::gonnect::VerificationEndEvent &endEvent);

    /// Request the initial room list (joined and unjoined rooms).
    void requestRoomList();

    /// Remove the room from the model and lookup, drop its notifications and delete it.
    void removeRoom(IpcChatRoom *room);

    /// Return the user room state cache for the given room, creating it if necessary.
    QHash<QString, IChatRoom::UserRoomState> *ensureUserRoomStateCache(const QString &roomId);

    /// If the tag belongs to a pending single message request, mark that message as failed so it
    /// will not be requested again.
    void markSingleMessageFailed(quint64 tag);

    bool hasOwnUserMention(const ChatMessage &message) const;
    ChatMessage *addReceivedChatMessage(const de::gonicus::gonnect::Message &message, bool isUnread,
                                        bool isIndependent);

    IpcChatRoom *addChatRoom(const de::gonicus::gonnect::Room &room, const QString &tag = "");

    IpcChatRoom *ipcChatRoomById(const QString &roomId) const;

    /// Ensure that a data folder exists for this configuration and return the absolute path.
    QString ensureDataFolderExists();

    QString uploadFolderPath() const;
    QString makeDataRootPath(const QString &subPath) const;
    QString makeRelativeToDataRootPath(const QString &path) const;

    IpcChatRoom *ipcDirectChatRoomForUser(const ChatUser *user) const;

    void setIsInVerificationProcess(bool value);
    void setIsDeviceVerified(bool value);

    /// Title and message body of the desktop notification for the given message, depending on its
    /// content type.
    std::pair<QString, QString> notificationTitleAndMessage(ChatMessage *messageObj,
                                                            IChatRoom *chatRoom) const;

    void makeNotificationNewMessage(ChatMessage *messageObj);
    void removeNotificationsForRoom(IChatRoom *room);

    bool shallSendDesktopNotification();

    QRegularExpression m_idConvRegex;
    bool m_useIdConversion = false;
    bool m_isInitialized = false;
    bool m_areCapabilitesInitialized = false;
    bool m_supportsDirectRooms = false;
    bool m_supportsGroupRooms = false;
    bool m_supportsSubThreads = false;
    bool m_hasFavoriteRooms = false;
    QStringList m_supportedMimeTypes;

    QObject *m_globalPresenceStateContext = nullptr;
    IpcInterface m_ipc;
    QSet<IpcConfig::LoginFlow> m_supportedLoginFlows;
    QStringList m_identityProviders;
    QString m_dataFolderPath;
    IpcConfig m_configInfo;
    QProtobufSerializer m_protoSerializer;
    quint64 m_nextFreeTag = 1;
    ConnectionState m_connectionState = ConnectionState::Disconnected;
    QList<IpcChatRoom *> m_rooms;
    QHash<QString, IpcChatRoom *> m_roomLookup;
    QString m_nextPublicRoomListResponseToken;
    QHash<IChatRoom *, QList<Notification *> *> m_chatNotifications;
    QTimer m_unreadUpdateTimer;

    bool m_hasDeviceVerification = false;
    bool m_isDeviceVerified = false;
    bool m_isRecoveryKeyVerificationAvailable = false;
    bool m_isCrossSigningVerificationAvailable = false;
    bool m_isInVerificationProcess = false;
    QString m_verificationFlowId;

    NotificationSetting::Setting m_notificationSetting = NotificationSetting::Setting::All;

    QSet<QString> m_requestedUserIds;
    QList<const ChatUser *> m_userList;

    /// Map roomId -> (userId -> user room state) of the last known information about the users
    /// state in the given room. This is useful when such information is received, but the user is
    /// not known yet and must be requested. On that response, the information can be taken from
    /// this.
    QHash<QString, QHash<QString, IChatRoom::UserRoomState> *> m_userRoomStateCache;

    /// Map of chat users, i.e. persons that somehow appear in chats, identified by their id.
    /// This IpcDispatcher object holds the ownership of all users related to one of its chat
    /// rooms.
    QHash<QString, ChatUser *> m_users;

    /// Map of chat users and their contact if it was found in the address book. If not, the
    /// id might still appear in this map, but the value is a nullptr. The address book retains
    /// ownership over the contact objects themselves.
    QHash<QString, Contact *> m_userContacts;

    /// Hash map of tags (≠ 0) of requests to QTimer objects that will trigger a timeout error.
    /// After receiving a response with a tag, the timer will be destroyed and removed from this
    /// map.
    QHash<quint64, QTimer *> m_timeoutTimers;

    /// Special timer for timeouts in the verification process, which are different from regular
    /// request timeouts.
    QTimer m_verificationTimeoutTimer;

    /// Map of ipc tag to room id of RoomMessagesRequest objects that have not received an answer
    /// yet.
    QHash<quint64, QString> m_roomListTags;

    /// Map of chat messages that have been requested indvidually (i.e. not in bulk) and have not
    /// received an answer yet. Key is the request tag, value the message id.
    QHash<quint64, QString> m_singleMessageTags;

    /// Map of ipc tag to a count of the parts that have been received for this tag, w/o the
    /// MultipartEnd message.
    QHash<quint64, qsizetype> m_multipartCount;

    /// Message IDs whose single-message request has failed or timed out. Prevents retry-spam.
    QSet<QString> m_failedMessageIds;

Q_SIGNALS:

    /// The IpcDispatcher, the sub process and the local socket communication have been successfully
    /// initialized. Does not say anything about the login/connection state of the client to the
    /// server. Use isInitialized() to retrieve the current state.
    void initializedChanged();

    /// The capabilites of the backend have been received. Check areCapabilitesInitialized() for the
    /// current state.
    void capabilitiesInitializedChanged();

    /// The login/connection state between client and server has changed. Use connectionState() to
    /// retrieve the current state.
    void connectionStateChanged();

    /// One or more reactions of the message have been changed.
    void reactionChanged(QString messageId);
};
