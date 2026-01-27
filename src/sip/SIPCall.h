#pragma once
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QDateTime>
#include <pjsua2.hpp>

#include "ICallState.h"
#include "ResponseItem.h"

class SIPAccount;
class CallHistoryItem;
class IMHandler;
class HeadsetDeviceProxy;

class SIPCall : public ICallState, public pj::Call
{
    Q_OBJECT
    Q_DISABLE_COPY(SIPCall)

public:
    explicit SIPCall(SIPAccount *account, int callId = PJSUA_INVALID_ID,
                     const QString &contactId = "", bool silent = false);
    virtual ~SIPCall();

    virtual void onCallState(pj::OnCallStateParam &prm) override;
    virtual void onCallTransferRequest(pj::OnCallTransferRequestParam &prm) override;
    virtual void onCallReplaceRequest(pj::OnCallReplaceRequestParam &prm) override;
    virtual void onCallMediaState(pj::OnCallMediaStateParam &prm) override;
    virtual void onInstantMessage(pj::OnInstantMessageParam &prm) override;
    virtual void onInstantMessageStatus(pj::OnInstantMessageStatusParam &prm) override;
    virtual void onDtmfDigit(pj::OnDtmfDigitParam &prm) override;
    virtual void onCallTsxState(pj::OnCallTsxStateParam &prm) override;

    SIPAccount *account() const { return m_account; };
    pj::AudioMedia *audioMedia() const;

    bool hasCapability(const QString &capability) const;
    bool triggerCapability(const QString &capability);

    QString notificationRef() { return m_notificationRef; }
    void setNotificationRef(const QString &ref) { m_notificationRef = ref; }

    QString sipUrl() const { return m_sipUrl; }
    bool isEmergencyCall() const;
    void setIncoming(bool flag) { m_incoming = flag; }
    bool isIncoming() const { return m_incoming; }

    void call(const QString &dst_uri, const pj::CallOpParam &prm);

    void addMetadata(const QString &data);
    bool hasMetadata() const { return m_hasMetadata; }
    QList<ResponseItem *> metadata() { return m_metadata; };


    /// Call party send IM with timestamp and dtmf digit it has sent
    void initializeCallDelay(qint64 timestamp, QString digit);
    /// Dtmf digit receival is logged with timestamp, followed by determining the delay
    void calculateCallDelay(qint64 timestamp, QString digit);

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

    bool earlyCallState() const { return m_earlyCallState; }

    virtual ContactInfo remoteContactInfo() const override { return m_contactInfo; }

protected:
    virtual void toggleHoldImpl() override;

Q_SIGNALS:
    void missed();
    void ringing();
    void establishedChanged();
    void earlyCallStateChanged();
    void isHoldingChanged();
    void isBlockedChanged();
    void capabilitiesChanged();
    void contactChanged();
    void metadataChanged();

private Q_SLOTS:
    void updateIsBlocked();
    void updateMutedState();

private:
    void setIsHolding(bool value);
    void setIsBlocked(bool value);
    void setContactInfo(const QString &sipUrl, bool isIncoming = true);
    void createOngoingCallNotification();

    ContactInfo m_contactInfo;
    SIPAccount *m_account = nullptr;
    QPointer<CallHistoryItem> m_historyItem;
    QDateTime m_establishedTime;

    QList<ResponseItem *> m_metadata;

    QTimer m_callDelayCycleTimer;

    struct CallDelay
    {
        qint64 sent;
        qint64 received;
        qint64 latency;
        QString digit;
    };

    CallDelay m_callDelay;

    pj::AudioMedia *m_aud_med = NULL;
    IMHandler *m_imHandler = nullptr;

    bool m_incoming = false;
    bool m_isEstablished = false;
    bool m_wasEstablished = false;
    bool m_managerNotified = false;
    bool m_isHolding = false;
    bool m_earlyCallState = false;
    bool m_isSilent = false;
    bool m_isBlocked = false;
    bool m_isEmergencyCall = false;
    bool m_hasMetadata = false;
    bool m_hasAccepted = false;
    bool m_hasRejected = false;

    QString m_sipUrl;
    QString m_contactId;
    QString m_notificationRef;
    QString m_postTask;
};
