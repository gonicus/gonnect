#pragma once
#include <QObject>
#include <QPointer>
#include <QDateTime>
#include <QTimer>
#include <pjsua2.hpp>

#include "ICallState.h"
#include "ResponseItem.h"
#include "SIPCallManager.h"

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
    ~SIPCall();

    void onCallState(pj::OnCallStateParam &prm) override;
    void onCallTransferRequest(pj::OnCallTransferRequestParam &prm) override;
    void onCallReplaceRequest(pj::OnCallReplaceRequestParam &prm) override;
    void onCallMediaState(pj::OnCallMediaStateParam &prm) override;
    void onInstantMessage(pj::OnInstantMessageParam &prm) override;
    void onInstantMessageStatus(pj::OnInstantMessageStatusParam &prm) override;
    void onCallTsxState(pj::OnCallTsxStateParam &prm) override;

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

    ContactInfo remoteContactInfo() const override { return m_contactInfo; }

    SIPCallManager::QualityLevel qualityLevel() const { return m_qualityLevel; }
    SIPCallManager::SecurityLevel securityLevel() const { return m_securityLevel; }

protected:
    void toggleHoldImpl() override;

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
    void rtcpStatsChanged();
    void qualityLevelChanged();
    void securityLevelChanged();

private Q_SLOTS:
    void updateIsBlocked();
    void updateMutedState();
    void updateRtcpStats();

private:
    void setIsHolding(bool value);
    void setIsBlocked(bool value);
    void setContactInfo(const QString &sipUrl, bool isIncoming = true);
    void setQualityLevel(SIPCallManager::QualityLevel qualityLevel);
    void setSecurityLevel(SIPCallManager::SecurityLevel securityLevel);
    void createOngoingCallNotification();
    float calculateMos(const pj::RtcpStreamStat &stat, int rttLast, double &jitter,
                       double &effectiveDelay);

    QTimer m_statsTimer;
    ContactInfo m_contactInfo;
    SIPAccount *m_account = nullptr;
    QPointer<CallHistoryItem> m_historyItem;
    QDateTime m_establishedTime;

    QList<ResponseItem *> m_metadata;

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

    SIPCallManager::QualityLevel m_qualityLevel = SIPCallManager::QualityLevel::High;
    SIPCallManager::SecurityLevel m_securityLevel = SIPCallManager::SecurityLevel::Low;

    QString m_codec;
    quint32 m_clockRate = 0;
    quint32 m_lastLoss = 0;
    quint32 m_lastPkt = 0;

    double m_mosRx = 0;
    double m_mosTx = 0;
    double m_lossTx = 0;
    double m_lossRx = 0;
    double m_jitterTx = 0;
    double m_jitterRx = 0;
    double m_effDelayTx = 0;
    double m_effDelayRx = 0;
    double m_mosTxLq = 0;
    double m_mosRxLq = 0;
    double m_mosTxCq = 0;
    double m_mosRxCq = 0;
    double m_jitterBufferTxDelay = 0;
    double m_jitterBufferRxDelay = 0;

    double m_rtt = 0;
    double m_lossRateTx = 0;
    double m_lossRateRx = 0;

    bool m_signalingEncrypted = false;
    bool m_mediaEncrypted = false;
};
