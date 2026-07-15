#pragma once

#include "IChatProvider.h"
#include "IpcConfig.h"
#include "IpcInterface.h"
#include "NotificationSetting.h"
#include "chat.qpb.h"
#include "IChatRoom.h"
#include "ChatUser.h"
#include "ChatMessageContentUserStateChange.h"

#include <QLocalServer>
#include <QLocalSocket>
#include <QPointer>
#include <QProcess>
#include <QProtobufSerializer>
#include <QTimer>
#include <QtEndian>
#include <QUrl>
#include <QRegularExpression>

class IpcChatRoom;
class ChatMessage;
class Contact;
class Notification;

#define GONNECT_IPC_TIMEOUT_SECS 29

template <typename T>
concept MessageDeliverer = requires(T obj) {
    { obj.hasText() } -> std::same_as<bool>;
    { obj.text() } -> std::convertible_to<const de::gonicus::gonnect::MessageContentText &>;
    { obj.hasFile() } -> std::same_as<bool>;
    { obj.file() } -> std::convertible_to<const de::gonicus::gonnect::MessageContentFile &>;
    { obj.hasMembershipChange() } -> std::same_as<bool>;
    {
        obj.membershipChange()
    } -> std::convertible_to<const de::gonicus::gonnect::MessageContentMembershipChange &>;
};

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

    /// Convert UserRoomState value from grpc to IChatRoom definition.
    static IChatRoom::UserRoomState
    userRoomStateConv(const de::gonicus::gonnect::UserRoomStateGadget::UserRoomState state);

    /// Convert PresenceState value from grpc to IChatRoom definition.
    static ChatUser::PresenceState
    presenceStateConv(const de::gonicus::gonnect::PresenceStateGadget::PresenceState state);

    /// Convert CrossSigningMethod method from grpc to CrossSigningSecret definition.
    static CrossSigningSecret::CrossSigningMethod crossSigningMethodConv(
            const de::gonicus::gonnect::CrossSigningMethodGadget::CrossSigningMethod method);

    /// Convert CrossSigningMethod method from CrossSigningSecret definition to grpc.
    static de::gonicus::gonnect::CrossSigningMethodGadget::CrossSigningMethod
    crossSigningMethodReConv(const CrossSigningSecret::CrossSigningMethod method);

    enum class ConnectionState {
        LoggedOut = static_cast<int>(de::gonicus::gonnect::StatusUpdate::StatusCode::LoggedOut),
        Connected = static_cast<int>(de::gonicus::gonnect::StatusUpdate::StatusCode::Connected),
        LoggedIn = static_cast<int>(de::gonicus::gonnect::StatusUpdate::StatusCode::LoggedIn),
        NetworkUnavailable = static_cast<int>(
                de::gonicus::gonnect::StatusUpdate::StatusCode::NetworkUnavailable),
        SessionInvalid =
                static_cast<int>(de::gonicus::gonnect::StatusUpdate::StatusCode::SessionInvalid),
    };
    Q_ENUM(ConnectionState)

    static de::gonicus::gonnect::NotificationSettingGadget::NotificationSetting
    notificationSettingIpcToProto(NotificationSetting::Setting setting);
    static NotificationSetting::Setting notificationSettingProtoToIpc(
            de::gonicus::gonnect::NotificationSettingGadget::NotificationSetting setting);

    static ::RoomSettings
    roomSettingsProtoToIpc(const de::gonicus::gonnect::RoomSettings &protoSettings);

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

    virtual void loadMessages(IChatRoom *chatRoom,
                              quint32 n = IChatProvider::defaultMessageLimit) override;

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
    virtual void retrySendMessage(const QString &roomId, const QString &failedMessageId) override;
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

    static IChatRoom::JoinRule
    joinRuleGrpcToGonnect(const de::gonicus::gonnect::RoomJoinRuleGadget::RoomJoinRule grpcRule);
    static de::gonicus::gonnect::RoomJoinRuleGadget::RoomJoinRule
    joinRuleGonnectToGrpc(const IChatRoom::JoinRule joinRule);

    static IChatRoom::LeaveReason leaveReasonGrpcToGonnect(
            const de::gonicus::gonnect::RoomLeftEvent::RoomLeaveReason leaveReason);
    static de::gonicus::gonnect::RoomLeftEvent::RoomLeaveReason
    leaveReasonGonnectToGrpc(const IChatRoom::LeaveReason leaveReason);

    static ChatMessageContentUserStateChange::State userStateGrpcToGonnect(
            de::gonicus::gonnect::MessageContentMembershipChange::MembershipChange change);

    static IChatRoom::Permissions
    roomPermissionsGrpcToGonnect(const de::gonicus::gonnect::RoomPermissions permissions);

    template <MessageDeliverer T>
    [[nodiscard]] QObject *createMessageContent(const T &message) const;

    /// Setup and start sub process.
    void init();

    /// Try to login with the information given in the IpcConfig struct. Should only be called when
    /// the headless client is ready to process the login, i.e. all forgoing information (like IdP)
    /// has been received.
    void login();

    /// Serialize and send a the request. This method takes ownership of the request object and
    /// destroys it after usage. If requestContainer has a tag, an answer to the request must be
    /// received within timeoutSeconds, or a timeout occurs. If the container has no tag or
    /// timeoutSeconds is 0, no timeout check is being made.
    /// The return value indicates whether the request has successfully been send. It does not
    /// represent a response to the request.
    bool sendRequest(de::gonicus::gonnect::RequestContainer *requestContainer,
                     quint32 timeoutSeconds = GONNECT_IPC_TIMEOUT_SECS);

    /// Create a new empty container message. Caller takes ownership of the returned object.
    /// If withTag is true, a unique tag is generated. Otherwise it is the zero id for fire and
    /// forget requests.
    [[nodiscard]] de::gonicus::gonnect::RequestContainer *createRequest(bool withTag = true);

    /// Dispatch the response container and its content payload.
    void processResponse(const de::gonicus::gonnect::ResponseContainer &responseContainer);

    bool hasOwnUserMention(const ChatMessage &message) const;
    ChatMessage *addOrUpdateReceivedChatMessage(const de::gonicus::gonnect::Message &message,
                                                bool isUnread, bool isIndependent,
                                                ChatMessage *chatMessage);

    IpcChatRoom *addChatRoom(const de::gonicus::gonnect::Room &room, const QString &tag = "");
    IpcChatRoom *addChatRoom(const QString &roomId, const QString &name, qsizetype unreadCount,
                             IChatRoom::JoinRule joinRule, bool isDirect, const QString &tag = "");

    IpcChatRoom *ipcChatRoomById(const QString &roomId) const;

    /// Ensure that a data folder exists for this configuration and return the absolute path.
    QString ensureDataFolderExists();

    QString uploadFolderPath() const;
    QString makeDataRootPath(const QString &subPath) const;
    QString makeRelativeToDataRootPath(const QString &path) const;

    IpcChatRoom *ipcDirectChatRoomForUser(const ChatUser *user) const;

    void setIsInVerificationProcess(bool value);
    void setIsDeviceVerified(bool value);

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
    ConnectionState m_connectionState = ConnectionState::LoggedOut;
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

    struct PendingMessageInfo
    {
        QString roomId;
        QString tempEventId;
    };

    /// Tags of pending (optimistic) messages that have been locally added but not yet confirmed.
    QHash<quint64, PendingMessageInfo> m_pendingMessages;

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
