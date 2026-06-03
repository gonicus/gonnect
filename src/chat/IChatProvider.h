#pragma once

#include <QObject>
#include <qqmlintegration.h>

#include "CrossSigningSecret.h"
#include "PublicChatRoom.h"
#include "ChatUser.h"

class IChatRoom;

class IChatProvider : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY isConnectedChanged FINAL)
    Q_PROPERTY(QString id READ id CONSTANT FINAL)
    Q_PROPERTY(QString displayName READ displayName CONSTANT FINAL)
    Q_PROPERTY(qsizetype unreadNotificationsCount READ unreadNotificationsCount NOTIFY
                       unreadNotificationsCountChanged FINAL)

    Q_PROPERTY(bool hasDeviceVerification READ hasDeviceVerification CONSTANT FINAL)
    Q_PROPERTY(bool isDeviceVerified READ isDeviceVerified NOTIFY isDeviceVerifiedChanged FINAL)
    Q_PROPERTY(bool isRecoveryKeyVerificationAvailable READ isRecoveryKeyVerificationAvailable
                       NOTIFY isRecoveryKeyVerificationAvailableChanged FINAL)
    Q_PROPERTY(bool isCrossSigningVerificationAvailable READ isCrossSigningVerificationAvailable
                       NOTIFY isCrossSigningVerificationAvailableChanged FINAL)
    Q_PROPERTY(bool isInVerificationProcess READ isInVerificationProcess NOTIFY
                       isInVerificationProcessChanged FINAL)
    Q_PROPERTY(bool hasFavoriteRooms READ hasFavoriteRooms NOTIFY hasFavoriteRoomsChanged FINAL)

    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    explicit IChatProvider(const QString &settingsGroup, QObject *parent = nullptr);
    virtual ~IChatProvider() { }

    QString id() const { return m_settingsGroup; }
    bool isConnected() const { return m_isConnected; }
    qsizetype unreadNotificationsCount() const { return m_unreadNotificationsCount; }

    virtual QString ownUserId() const = 0;
    virtual QString displayName() = 0;
    virtual QList<IChatRoom *> chatRooms() = 0;
    virtual bool hasFavoriteRooms() const = 0;
    virtual qsizetype indexOf(IChatRoom *chatRoom) const = 0;
    Q_INVOKABLE virtual IChatRoom *chatRoomByRoomId(const QString &roomId) const = 0;
    Q_INVOKABLE virtual QString chatRoomIdForUser(const ChatUser *user) const = 0;
    Q_INVOKABLE virtual QString chatRoomIdForUser(const QString &userId) const = 0;

    /// Search for chat users/users that match the search phrase. Result is a id for the
    /// search process. When finished, the signal chatUserSearchResult will be send with that id.
    virtual QString searchChatUser(const QString &searchPhrase) = 0;

    virtual QList<const ChatUser *> users() const = 0;
    Q_INVOKABLE virtual ChatUser *userById(const QString &userId) const = 0;

    /// Invite the users to the given room. The text is an optional text to show the users
    /// with the invitation.
    Q_INVOKABLE virtual void inviteUsers(const QString &roomId, const QList<QString> &userIds,
                                         const QString &text = "") = 0;

    /// Search for public rooms by the search text. Leaving the text empty will list all public
    /// rooms. Retun value is id for the search process and will be references in the finishing
    /// publicRoomSearchResult() signal.
    virtual QString searchPublicRoomRequest(const QString &searchText,
                                            const QString &offsetToken = "", quint32 limit = 0) = 0;

    /// Request the creation of a room with exactly one user. Therefore, the user id
    /// must not be empty. Whether the name can be empty has to be determined by the implementation.
    /// If the creation is successful, the signal chatRoomAdded() will be emitted once the room has
    /// been created.
    /// If supported, the method might return a tag which then shall match with the tag in the
    /// chatRoomAdded() signal to know when the specifically requested room has been created.
    Q_INVOKABLE virtual QString requestDirectRoomCreation(const QString &userId,
                                                          const QString &name = "",
                                                          const QString &avatarPath = "") = 0;

    /// Requests the creation of a room with an arbitrary number of users. Whether the list
    /// of user ids and/or the name can be empty has to be determined by the implementation.
    /// If the creation is successful, the signal chatRoomAdded() will be emitted once the room has
    /// been created.
    /// If supported, the method might return a tag which then shall match with the tag in the
    /// chatRoomAdded() signal to know when the specifically requested room has been created.
    Q_INVOKABLE virtual QString requestGroupRoomCreation(const QStringList &userIds,
                                                         const IChatRoom::JoinRule joinRule,
                                                         const QString &name = "",
                                                         const QString &avatarPath = "") = 0;

    /// Request to change attributes of the given room. The room object pointer must not be nullptr.
    Q_INVOKABLE virtual void requestRoomChange(IChatRoom *chatRoom, const QString &name,
                                               IChatRoom::JoinRule joinRule,
                                               const QString &avatarPath) = 0;

    /// Request to change the favorite status of room identified by its pointer, wich must not be
    /// nullptr.
    Q_INVOKABLE virtual void requestToggleRoomFavorite(IChatRoom *chatRoom) = 0;

    /// Request to join the given chat room. The room must be directly joinable, i.e. does not
    /// require another user to accept the request. If successful, a corresponding chatRoomJoined()
    /// signal is send afterwards.
    Q_INVOKABLE virtual void joinRoomRequest(const QString &roomId) = 0;

    /// Sends a response to a presceding invitation to a room, specified by the room id. The
    /// acceptInvitation must be true for accepting the invitation and false for declining it.
    /// Sending an answer to a non-existing invitation is undefined behaviour and must be avoided.
    Q_INVOKABLE virtual void respondToInvitation(const QString &roomId, bool acceptInvitation) = 0;

    /// Request to join a room that cannot be simply joined, but requires another user to accept the
    /// request. The message is an optional text by the knocking user to show to the other users
    /// that can accept or decline the request. If successful, a chatRoomJoined() signal is sent
    /// once the request has been accepted.
    Q_INVOKABLE virtual void knockRoomRequest(const QString &roomId,
                                              const QString &message = "") = 0;

    /// Request to leave the room specified by the room id given. If successful, the signal
    /// chatRoomLeft() must be emitted.
    Q_INVOKABLE virtual void requestRoomLeave(const QString &roomId) = 0;

    /// Request info about this user.
    Q_INVOKABLE virtual void requestUser(const QString &userId) = 0;

    /// Request to delete the message with the given id.
    Q_INVOKABLE virtual void requestRemoveMessage(const QString &roomId,
                                                  const QString &messageId) = 0;

    /// Request to edit (i.e. change the text content) the message with the given id. This method
    /// should not be called with an empty string - use requestRemoveMessage() instead.
    Q_INVOKABLE virtual void requestEditMessage(const QString &roomId, const QString &messageId,
                                                const QString &newContent) = 0;

    /// Request to add the user's reaction (e.g. emoji) for a specific message.
    Q_INVOKABLE virtual void addReaction(const QString &roomId, const QString &messageId,
                                         const QString &reaction) = 0;

    /// Request to retract an existing user reaction (e.g. emoji) for a specific message.
    Q_INVOKABLE virtual void retractReaction(const QString &roomId, const QString &messageId,
                                             const QString &reaction) = 0;

    /// If the reaction currently exist, it will call retractReaction() with the same parameters,
    /// and addReaction() if it does not exist. This should only be used if the current state is not
    /// known to prevent expensive lookups.
    Q_INVOKABLE virtual void toggleReaction(const QString &roomId, const QString &messageId,
                                            const QString &reaction) = 0;

    /// Upload a file to the backend and return a usable path to it.
    Q_INVOKABLE virtual QString uploadFile(const QString &filePath) = 0;

    /// Receive an image from the clipboard and send it as a message in the given room.
    Q_INVOKABLE virtual void uploadImageFromClipboard(const QString &roomId) = 0;

    /*! \name Device Verification
     *
     * Methods that deal with device verification. If verification mechanisms actually exist in this
     * type of provider is determined via hasDeviceVerification(). If it is false, using any other
     * method of this block is automatically invalid and must be avoided.
     *
     * Only one verification process at a time is supported; the current state can be requested via
     * isInVerificationProcess(). If the verification process has ended without an error, the
     * isDeviceVerified() might turn true if the verification has been successful. When an error
     * occurs, verificationError() will be send and isInVerificationProcess() must immediately turn
     * false.
     */
    ///@{

    /// Whether this provider generally has a concept of device verification. All other verification
    /// methods and signals are only available and being used if this is true.
    virtual bool hasDeviceVerification() const { return false; }

    /// Whether this provider is currently a verified device. The according signal is
    /// isDeviceVerifiedChanged. Only available when hasDeviceVerification() is true.
    virtual bool isDeviceVerified() const { return false; }

    /// Whether the recovery key verification method is currently available. The according signal is
    /// isRecoveryKeyVerificationAvailableChanged. Only available when hasDeviceVerification() is
    /// true.
    virtual bool isRecoveryKeyVerificationAvailable() const { return false; }

    /// Whether the cross signing verification method is currently available. The according signal
    /// is isCrossSigningVerificationAvailableChanged. Only available when hasDeviceVerification()
    /// is true.
    virtual bool isCrossSigningVerificationAvailable() const { return false; }

    /// Whether the provider is currently in a verification process. Only one such process can
    /// happen at a time. The according signal is isInVerificationProcessChanged().
    virtual bool isInVerificationProcess() const { return false; }

    /// Try to verify device with the given recovery key, under the premise that recovery key
    /// verification is available and the device is currently not verified. Is followed by either a
    /// verificationError() or an isInVerificationProcessChanged() (and an isDeviceVerifiedChanged()
    /// if successful) signal.
    Q_INVOKABLE virtual void requestRecoveryKeyVerification(const QString &key)
    {
        Q_UNUSED(key);
        Q_UNIMPLEMENTED();
    }

    /// Accept the ongoing verification once the secret has been verified by the user.
    Q_INVOKABLE virtual void acceptVerification() { Q_UNIMPLEMENTED(); }

    /// Cancel an ongoing verification process.
    Q_INVOKABLE virtual void requestVerificationAbort() { Q_UNIMPLEMENTED(); }

    /// Try to verify device by requesting to start cross signing. This can answer a
    /// crossSigningPrompt() or start a new process alltogether.
    Q_INVOKABLE virtual void requestCrossSigningStart() { Q_UNIMPLEMENTED(); }

    /// Select a method in response to a previous crossSigningMethodSelectRequired() signal.
    Q_INVOKABLE virtual void selectCrossSigningMethod(CrossSigningSecret::CrossSigningMethod method)
    {
        Q_UNUSED(method);
        Q_UNIMPLEMENTED();
    }

Q_SIGNALS:
    void isInVerificationProcessChanged();
    void verificationError(QString error);

    void isDeviceVerifiedChanged();
    void isRecoveryKeyVerificationAvailableChanged();
    void isCrossSigningVerificationAvailableChanged();

    /// Signals that another device wants to verify this one via cross signing. Must be answered by
    /// either calling requestVerificationAbort() to reject or requestCrossSigningStart() to accept.
    void crossSigningPrompt();

    /// Signals that one of the given cross signing methods must be chosen.
    /// selectCrossSigningMethod() should be called afterwards
    void crossSigningMethodSelectRequired(QList<CrossSigningSecret::CrossSigningMethod> methods);

    /// Signals that the secret should be displayed to the user who then must accept
    void crossSigningAcceptRequired(CrossSigningSecret *secret);

    ///@}

protected:
    void setIsConnected(bool value);
    void setUnreadNotificationsCount(qsizetype count);

    bool m_isConnected = false;
    QString m_settingsGroup;
    qsizetype m_unreadNotificationsCount = 0;

Q_SIGNALS:
    void isConnectedChanged();
    void hasFavoriteRoomsChanged();
    void unreadNotificationsCountChanged();

    /// Send when a chat room has been added at the given index in the list returned by chatRooms()
    /// or given by indexOf(). The tag can be set when responding to a specific request; otherwise
    /// it must be the empty string.
    void chatRoomAdded(qsizetype index, IChatRoom *room, QString tag = "");
    void chatRoomNameChanged(qsizetype index, IChatRoom *room, QString name);
    void chatRoomIsFavoriteChanged(qsizetype index, IChatRoom *room, bool isFavorite);
    void chatRoomAvatarPathChanged(qsizetype index, IChatRoom *room, QString avatarPath);
    void chatRoomJoinRuleChanged(qsizetype index, IChatRoom *room, IChatRoom::JoinRule joinRule);
    void chatRoomNotificationCountChanged(qsizetype index, IChatRoom *room, qsizetype count);
    void chatRoomLatestActivityChanged(qsizetype index, IChatRoom *room, QDateTime dateTime);
    void chatRoomPermissionsChanged(qsizetype index, IChatRoom *room,
                                    IChatRoom::Permissions permissions);
    void chatRoomRemoved(qsizetype index, IChatRoom *room);
    void chatRoomOwnJoinStateChanged(qsizetype index, IChatRoom *room,
                                     IChatRoom::UserRoomState userRoomState);
    void chatRoomTypingChanged(qsizetype index, IChatRoom *room);

    /// Signals that the client has joined the chat room.
    void chatRoomJoined(QString roomId);

    /// Send when the user has left a chat room - either on their own wish or for any other reason.
    void chatRoomLeft(QString roomId, QString roomName, IChatRoom::LeaveReason leaveReason,
                      QString message = "");

    /// Fired when at least one of the properties (name, image, etc.) of a user has changed.
    /// If a chat room for direct communication with this user is known, the second parameter
    /// is that room or nullptr otherwise. The index is that in the list as returned by chatRooms()
    /// or -1.
    void chatUserPropertiesChanged(ChatUser *user, IChatRoom *chatRoom, qsizetype index);

    /// Send whenever a new user object is added to this dispatcher. That does not mean that
    /// the user itself is new. The ownership remains with this IpcDispatcher.
    /// The index is the user's position in the list returned by users().
    void userAdded(QString id, ChatUser *user, qsizetype index);

    /// Send whenever a user object is removed from the dispatcher. That does not mean that
    /// the user itself has been removed from the backend. It might also be that the client
    /// is not permitted to see it any longer or it is not neccessary, because all its appearances
    /// in rooms, buddy lists etc. have been deleted. The object is immediately deleted after
    /// sending the signal and should not be modified in any way by a receiver.
    /// /// The index is the user's position in the list returned by users().
    void userRemoved(QString id, ChatUser *user, qsizetype index);

    /// Result of a user search initialized via searchChatUser() with the given search id and the
    /// resulting list.
    void chatUserSearchResult(QString searchId, ::QList<::ChatUser *> userList);

    ///  Result of a public room search via searchPublicRoomRequest() with the given search id.
    void publicRoomSearchResult(QString searchId, QList<QSharedPointer<PublicChatRoom>> roomList,
                                QString nextBatchToken = "");

    /// Received invitation to a room.
    void roomInviteReceived(QString roomId, QString roomDisplayName, QString invitationText);

    /// An image upload from the clipboard has finished. The image in imageFilePath is ready to be
    /// reviewed and/or sent.
    void clipboardImageUploaded(QUrl imageFilePath, IChatRoom *chatRoom);
};
