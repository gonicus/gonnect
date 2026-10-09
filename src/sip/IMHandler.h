#pragma once
#include <QObject>
#include <QPointer>
#include <QRegularExpression>

class SIPCall;
class CallHistoryItem;
class Contact;

class IMHandler : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(IMHandler)

public:
    explicit IMHandler(SIPCall *parent);
    ~IMHandler();

    bool jitsiEnabled() const
    {
        return !m_jitsiBaseURL.isEmpty() && m_capabilities.contains("jitsi");
    }
    bool process(const QString &contentType, const QString &message);

    bool capabilitiesSent() const { return m_capabilitiesSent; }
    bool sendCapabilities();
    bool triggerCapability(const QString &capability,
                           QPointer<CallHistoryItem> callHistoryItem = QPointer<CallHistoryItem>());

    bool hasCapability(const QString &capability) const;

    void openMeeting(const QString &meetingId, const QString &displayName = "", bool hangup = false,
                     QPointer<CallHistoryItem> callHistoryItem = QPointer<CallHistoryItem>(),
                     QPointer<Contact> contact = QPointer<Contact>());

    void handleDtmfDigit(const QString &digit);

Q_SIGNALS:
    void capabilitiesChanged();
    void meetingRequested(const QString &accountId, int callId);

private:
    struct ForcedUpgradeRequest
    {
        bool hangup = false;
        QPointer<CallHistoryItem> callHistoryItem;
        QString displayName;
        QPointer<Contact> contact;
    };

    bool requestMeeting(bool hangup,
                        QPointer<CallHistoryItem> callHistoryItem = QPointer<CallHistoryItem>(),
                        const QString &displayName = "",
                        QPointer<Contact> contact = QPointer<Contact>());

    bool requestForcedUpgrade(bool hangup, QPointer<CallHistoryItem> callHistoryItem,
                              const QString &displayName, QPointer<Contact> contact);
    bool forcedUpgradeEnabled() const;
    QString forcedUpgradeRoomName() const;

    void migrationHangup();

    SIPCall *m_call = nullptr;

    QString m_pendingHangupMeetingId;
    QMetaObject::Connection m_meetingEstablishedConn;
    bool m_migrationHangupDone = false;

    QStringList m_capabilities;
    QStringList m_ownCapabilities;
    QString m_jitsiBaseURL;
    QString m_jistiRequestedMeetingId;

    unsigned m_capabilitySendingTries = 3;

    bool m_capabilitiesSent = false;
    bool m_jitsiPreconfig = false;

    QRegularExpression m_forcedUpgradeNumberPattern;
    std::optional<ForcedUpgradeRequest> m_pendingForcedUpgrade;
};
