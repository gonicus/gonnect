#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QStandardPaths>

#include "appversion.h"
#include "Application.h"
#include "NumberStats.h"
#include "JitsiConnector.h"

class Ringer;
class Contact;
class HeadsetDeviceProxy;
class CallHistoryItem;

class ViewHelper : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isJitsiAvailable READ isJitsiAvailable CONSTANT FINAL)
    Q_PROPERTY(QString userConfigPath READ userConfigPath CONSTANT FINAL)
    Q_PROPERTY(bool isDebugRun READ isDebugRun CONSTANT FINAL)
    Q_PROPERTY(bool isPlayingRingTone READ isPlayingRingTone NOTIFY isPlayingRingToneChanged FINAL)
    Q_PROPERTY(Contact *currentUser READ currentUser NOTIFY currentUserChanged FINAL)
    Q_PROPERTY(QString currentUserName READ currentUserName NOTIFY currentUserChanged FINAL)
    Q_PROPERTY(IConferenceConnector::StartFlags nextMeetingStartFlags MEMBER m_nextMeetingStartFlags
                       NOTIFY nextMeetingStartFlagsChanged FINAL)
    Q_PROPERTY(QObject *topDrawer MEMBER m_topDrawer NOTIFY topDrawerChanged FINAL)
    Q_PROPERTY(bool isActiveVideoCall READ isActiveVideoCall NOTIFY isActiveVideoCallChanged FINAL)
    Q_PROPERTY(bool unsupportedPlatform READ isUnsupportedPlatform CONSTANT FINAL)

public:
    static ViewHelper &instance()
    {
        static ViewHelper *_instance = nullptr;
        if (!_instance) {
            _instance = new ViewHelper;
        }
        return *_instance;
    }

    Q_INVOKABLE QString appVersion() const { return QString::fromStdString(getVersion()); }

    Q_INVOKABLE bool isSystrayAvailable() const;

    bool isJitsiAvailable() const;

    bool isDebugRun() const
    {
        return qobject_cast<Application *>(Application::instance())->isDebugRun();
    }
    Q_INVOKABLE void downloadDebugInformation() const;

    QString userConfigPath() const
    {
        return "file:///" + QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                + "/gonnect/99-user.conf";
    }

    Q_INVOKABLE bool isToday(const QDate date) const;
    Q_INVOKABLE bool isTomorrow(const QDate date) const;
    Q_INVOKABLE QString minutesToNiceText(uint minutes) const;
    Q_INVOKABLE QString secondsToNiceText(int seconds) const;
    Q_INVOKABLE int secondsDelta(const QDateTime &start, const QDateTime &end) const;
    Q_INVOKABLE void copyToClipboard(const QString &str) const;
    Q_INVOKABLE void reloadAddressBook() const;

    /// Returns the contact id for the phone number if such contact exists or empty string
    Q_INVOKABLE QString contactIdByNumber(const QString &phoneNumber) const;

    /// Returns the contact for the given source id (e.g. IPA, EDS) or null if not found
    Q_INVOKABLE Contact *contactIdBySourceUid(const QString &sourceUid) const;

    Contact *currentUser() const { return m_currentUser; }
    QString currentUserName() const;

    /// List of file name filters for supported audio files (as used by a file picker dialog)
    Q_INVOKABLE QStringList audioFileSelectors() const;

    Q_INVOKABLE void toggleFavorite(const QString &phoneNumber,
                                    const NumberStats::ContactType contactType) const;

    /// Creates initials (two letters) out of the given name
    Q_INVOKABLE QString initials(const QString &name) const;

    Q_INVOKABLE void testPlayRingTone(qreal volume);
    Q_INVOKABLE void stopTestPlayRingTone();
    bool isPlayingRingTone() const { return m_isPlayingRingTone; }

    Q_INVOKABLE void resetTrayIcon() const;

    Q_INVOKABLE HeadsetDeviceProxy *headsetDeviceProxy() const;

    void requestPassword(const QString &id, const QString &host);
    Q_INVOKABLE void respondPassword(const QString &id, const QString password);

    void requestRecoveryKey(const QString &id, const QString &displayName);
    Q_INVOKABLE void respondRecoveryKey(const QString &id, const QString &key);

    Q_INVOKABLE uint durationCallVisibleAfterEnd() const { return GONNECT_CALL_VISIBLE_AFTER_END; }

    const QString requestUserVerification(const QString &verificationKey);
    Q_INVOKABLE void respondUserVerification(const QString &uuid, bool isAccepted);

    Q_INVOKABLE bool isPhoneNumber(const QString &number) const;
    Q_INVOKABLE bool isValidJitsiRoomName(const QString &name) const;

    bool isUnsupportedPlatform() const;

    Q_INVOKABLE void
    requestMeeting(const QString &roomName,
                   QPointer<CallHistoryItem> callHistoryItem = QPointer<CallHistoryItem>(),
                   const QString &displayName = "");

    Q_INVOKABLE void setCallInForegroundByIds(const QString &accountId, int callId);

    bool isActiveVideoCall() const { return m_isActiveVideoCall; }
    Q_INVOKABLE bool hasNonSilentCall() const;

    Q_INVOKABLE bool isBusyOnBusy() const;

    Q_INVOKABLE bool hasOngoingDateEventByRoomName(const QString &roomName) const;
    Q_INVOKABLE QDateTime endTimeForOngoingDateEventByRoomName(const QString &roomName) const;

    Q_INVOKABLE void toggleFullscreen();

public Q_SLOTS:
    Q_INVOKABLE void quitApplicationNoConfirm() const;
    Q_INVOKABLE void quitApplication();

private Q_SLOTS:
    void updateCurrentUser();
    void updateIsActiveVideoCall();

private:
    explicit ViewHelper(QObject *parent = nullptr);

    bool m_isPlayingRingTone = false;
    Ringer *m_ringer = nullptr;
    QTimer m_ringerTimer;
    Contact *m_currentUser = nullptr;
    IConferenceConnector::StartFlags m_nextMeetingStartFlags =
            IConferenceConnector::StartFlag::AudioActive;
    QObject *m_topDrawer = nullptr;
    bool m_isActiveVideoCall = false;

Q_SIGNALS:
    void activateSearch();
    void isPlayingRingToneChanged();
    void currentUserChanged();
    void nextMeetingStartFlagsChanged();
    void topDrawerChanged();
    void isActiveVideoCallChanged();

    void showSettings();
    void showAudioSettings();
    void showTutorial();
    void showShortcuts();
    void showAbout();
    void showDialPad();
    void showFirstAid();
    void showQuitConfirm();
    void showEmergency(QString accountId, int callId, QString displayName);
    void hideEmergency();
    void showConferenceChat();
    void fullscreenToggle();

    void openMeetingRequested(QString meetingId, QString displayName,
                              IConferenceConnector::StartFlags startFlags,
                              QPointer<CallHistoryItem> callHistoryItem);

    void passwordRequested(QString id, QString host);
    void passwordResponded(QString id, QString password);

    void recoveryKeyRequested(QString id, QString displayName);
    void recoveryKeyResponded(QString id, QString key);

    void userVerificationRequested(QString id, QString verificationKey);
    void userVerificationResponded(QString id, bool isAccepted);
};

class ViewHelperWrapper
{
    Q_GADGET
    QML_FOREIGN(ViewHelper)
    QML_NAMED_ELEMENT(ViewHelper)
    QML_SINGLETON

public:
    static ViewHelper *create(QQmlEngine *, QJSEngine *) { return &ViewHelper::instance(); }

private:
    ViewHelperWrapper() = default;
};
