#pragma once
#include <QObject>
#include <QPointer>
#include <QDateTime>
#include <pjsua2.hpp>

class SIPAccount;
class CallHistoryItem;
class IMHandler;
class HeadsetDeviceProxy;

class SIPCall : public QObject, public pj::Call
{
    Q_OBJECT
    Q_DISABLE_COPY(SIPCall)

public:
    explicit SIPCall(SIPAccount *account, int callId = PJSUA_INVALID_ID,
                     const QString &contactId = "", bool silent = false);
    virtual ~SIPCall();

    virtual void onCallState(pj::OnCallStateParam &prm);
    virtual void onCallTransferRequest(pj::OnCallTransferRequestParam &prm);
    virtual void onCallReplaceRequest(pj::OnCallReplaceRequestParam &prm);
    virtual void onCallMediaState(pj::OnCallMediaStateParam &prm);
    virtual void onInstantMessage(pj::OnInstantMessageParam &prm);
    virtual void onInstantMessageStatus(pj::OnInstantMessageStatusParam &prm);

    SIPAccount *account() const { return m_account; };
    pj::AudioMedia *audioMedia() const;

    bool hasCapability(const QString &capability) const;
    bool triggerCapability(const QString &capability) const;

    QString notificationRef() { return m_notificationRef; }
    void setNotificationRef(const QString &ref) { m_notificationRef = ref; }

    QString sipUrl() const { return m_sipUrl; }
    bool isEmergencyCall() const;
    void setIncoming(bool flag) { m_incoming = flag; }
    bool isIncoming() const { return m_incoming; }

    void call(const QString &dst_uri, const pj::CallOpParam &prm);

    bool hold();
    bool unhold();
    bool isHolding() const { return m_isHolding; }

    bool isSilent() const { return m_isSilent; }
    bool isBlocked() const { return m_isBlocked; }

    void accept();
    void reject();

    bool isEstablished() const { return m_isEstablished; }
    /// The time when the call was established (i.e. answered); invalid QDateTime if not established
    QDateTime establishedTime() const { return m_establishedTime; }

    bool earlyMediaActive() const { return m_earlyMediaActive; }

signals:
    void missed();
    void ringing();
    void establishedChanged();
    void earlyMediaActiveChanged();
    void isHoldingChanged();
    void isBlockedChanged();
    void capabilitiesChanged();
    void contactChanged();

private slots:
    void updateIsBlocked();

private:
    void setIsHolding(bool value);
    void setIsBlocked(bool value);
    void setContactInfo(const QString &sipUrl, bool isIncoming = true);
    void createOngoingCallNotification();
    bool hasAllow(const QString &header) const;
    bool hasAllowGrant(const QString &header, const QString &grant) const;

    SIPAccount *m_account = nullptr;
    QPointer<CallHistoryItem> m_historyItem;
    QDateTime m_establishedTime;

    pj::AudioMedia *m_aud_med = NULL;
    IMHandler *m_imHandler = nullptr;
    HeadsetDeviceProxy *m_proxy = nullptr;

    bool m_incoming = false;
    bool m_isEstablished = false;
    bool m_wasEstablished = false;
    bool m_managerNotified = false;
    bool m_isHolding = false;
    bool m_earlyMediaActive = false;
    bool m_isSilent = false;
    bool m_isBlocked = false;
    bool m_isEmergencyCall = false;
    bool m_isMessageAllowed = false;

    QString m_sipUrl;
    QString m_contactId;
    QString m_notificationRef;
    QString m_postTask;
};
