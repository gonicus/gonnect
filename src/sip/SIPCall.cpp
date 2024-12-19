#include <QLoggingCategory>
#include <QRegularExpression>

#include "SIPCall.h"
#include "SIPCallManager.h"
#include "SIPAccount.h"
#include "SIPAudioManager.h"
#include "RingToneFactory.h"
#include "RingTone.h"
#include "EnumTranslation.h"
#include "CallHistory.h"
#include "CallHistoryItem.h"
#include "NumberStats.h"
#include "IMHandler.h"
#include "media/Sniffer.h"
#include "ViewHelper.h"
#include "HeadsetDeviceProxy.h"
#include "HeadsetDevices.h"
#include "Notification.h"
#include "NotificationManager.h"
#include "AvatarManager.h"

#include "pjsua-lib/pjsua.h"

Q_LOGGING_CATEGORY(lcSIPCall, "gonnect.sip.call")

using namespace std::chrono_literals;

SIPCall::SIPCall(SIPAccount *account, int callId, const QString &contactId, bool silent)
    : QObject(account),
      pj::Call(*account, callId),
      m_account(account),
      m_isSilent(silent),
      m_contactId(contactId)
{

    auto &sipCallManager = SIPCallManager::instance();
    sipCallManager.addCall(this);

    m_imHandler = new IMHandler(this);
    connect(m_imHandler, &IMHandler::capabilitiesChanged, this, &SIPCall::capabilitiesChanged);
    connect(m_imHandler, &IMHandler::meetingRequested, [](const QString &accountId, int callId) {
        emit SIPCallManager::instance().meetingRequested(accountId, callId);
    });

    m_proxy = HeadsetDevices::instance().getProxy();

    // Initialize basic call info
    // This can only be done here for incoming calls, because an outgoing call has its infos not set
    // yet. But incoming calls must be treated here to check whether the contact/number is blocked
    if (callId >= 0) {
        const pj::CallInfo ci = getInfo();
        const auto remoteUri = QString::fromStdString(ci.remoteUri);

        if (!m_isSilent && !m_historyItem) {
            setContactInfo(remoteUri, ci.role != PJSIP_ROLE_UAC);
            updateIsBlocked();
        }
    }

    connect(this, &SIPCall::isBlockedChanged, this,
            [this]() { emit SIPCallManager::instance().isBlockedChanged(this); });
    connect(&sipCallManager, &SIPCallManager::blocksChanged, this, &SIPCall::updateIsBlocked);
}

SIPCall::~SIPCall()
{
    if (m_isEmergencyCall) {
        emit ViewHelper::instance().hideEmergency();
    }

    if (!m_incoming) {
        auto &ringToneFactory = RingToneFactory::instance();
        ringToneFactory.ringingTone()->stop();
    }

    if (m_historyItem && m_wasEstablished) {
        m_historyItem->endCall();
    }

    SIPCallManager::instance().removeCall(this);
}

void SIPCall::call(const QString &dst_uri, const pj::CallOpParam &prm)
{
    // Extract "," DTMF string and store them for later playback
    m_postTask = dst_uri.section(',', 1, -1, QString::SectionIncludeLeadingSep);

    makeCall(dst_uri.toStdString(), prm);
}

void SIPCall::onCallState(pj::OnCallStateParam &prm)
{
    Q_UNUSED(prm)

    if (!m_managerNotified) {
        SIPCallManager::instance().updateCallCount();
        m_managerNotified = true;
    }

    const pj::CallInfo ci = getInfo();
    const auto remoteUri = QString::fromStdString(ci.remoteUri);

    if (!m_isSilent && !m_historyItem) {
        setContactInfo(remoteUri, ci.role != PJSIP_ROLE_UAC);
        updateIsBlocked();
    }

    auto &ringToneFactory = RingToneFactory::instance();

    const auto statusCode = ci.lastStatusCode;
    qCInfo(lcSIPCall).nospace() << "Call State: " << ci.stateText << " (" << remoteUri << ")"
                                << " last status code: "
                                << EnumTranslation::instance().sipStatusCode(statusCode) << " ("
                                << statusCode << ") "
                                << " last reason " << ci.lastReason << " contactId " << m_contactId;

    switch (ci.state) {
    case PJSIP_INV_STATE_NULL:
        if (!m_isSilent) {
            ringToneFactory.ringingTone()->stop();
        }
        break;

    case PJSIP_INV_STATE_CONNECTING:
    case PJSIP_INV_STATE_CALLING:
        if (!m_isSilent && !m_incoming) {
            ringToneFactory.ringingTone()->start();
        }
        break;

    case PJSIP_INV_STATE_INCOMING:
        if (!m_isSilent && !m_incoming) {
            ringToneFactory.ringingTone()->start();
        }
        break;

    case PJSIP_INV_STATE_EARLY:
        if (!m_isSilent && !m_incoming) {
            m_wasEstablished = true;
            m_earlyMediaActive = true;
            emit earlyMediaActiveChanged();
        }
        break;

    case PJSIP_INV_STATE_CONFIRMED:
        if (!m_isSilent) {
            ringToneFactory.ringingTone()->stop();
            m_isEstablished = true;
            m_wasEstablished = true;
            m_establishedTime = QDateTime::currentDateTime();

            if (isEmergencyCall()) {
                emit ViewHelper::instance().hideEmergency();
            }
            emit establishedChanged();

            m_proxy->setBusyLine(true);

            m_earlyMediaActive = false;
            emit earlyMediaActiveChanged();

            if (!m_imHandler->capabilitiesSent()) {
                QTimer::singleShot(2s, this, [this]() { m_imHandler->sendCapabilities(); });
            }

            createOngoingCallNotification();
        }

        // Send DTMF post tasks if present
        if (!m_postTask.isEmpty()) {
            auto &cm = SIPCallManager::instance();
            QString task = m_postTask;
            cm.sendDtmf(m_account->id(), getId(), task);
            m_postTask = "";
        }

        break;

    case PJSIP_INV_STATE_DISCONNECTED:
        if (!m_isEstablished && m_incoming) {
            emit missed();
        }

        if (m_isEstablished && SIPCallManager::instance().calls().count() == 1) {
            m_proxy->setIdle();
        }

        qCInfo(lcSIPCall).nospace() << "Call state disconnected, reason: " << ci.lastReason << ", "
                                    << EnumTranslation::instance().sipStatusCode(statusCode) << " ("
                                    << statusCode << ")";

        ringToneFactory.ringingTone()->stop();

        if (!m_isSilent) {
            if (m_isEstablished) {
                ringToneFactory.endTone()->start();
            }
            if (!m_incoming) {
                if (statusCode == PJSIP_SC_BUSY_HERE) {
                    ringToneFactory.busyTone()->start(5);
                } else if (static_cast<int>(statusCode) >= 400
                           && static_cast<int>(statusCode) < 700) {
                    ringToneFactory.congestionTone()->start(5);
                }
            }
        }

        if (isEmergencyCall()) {
            emit ViewHelper::instance().hideEmergency();
        }

        m_account->removeCall(this);
        m_isEstablished = false;
        m_earlyMediaActive = false;

        break;

    default:
        break;
    }

    qCInfo(lcSIPCall) << "Emitting call state of call" << getId() << "and status" << statusCode;
    emit SIPCallManager::instance().callState(getId(), statusCode);
}

void SIPCall::onCallMediaState(pj::OnCallMediaStateParam &prm)
{
    Q_UNUSED(prm);

    auto &audManager = SIPAudioManager::instance();

    pj::CallInfo ci = getInfo();
    pj::AudioMedia aud_med;

    pj::AudioMedia &speaker_media = audManager.getPlaybackDevMedia();
    pj::AudioMedia &mic_media = audManager.getCaptureDevMedia();

    for (unsigned i = 0; i < ci.media.size(); ++i) {

        const auto &mediaInfo = ci.media[i];

        if (mediaInfo.type == PJMEDIA_TYPE_AUDIO) {

            switch (mediaInfo.status) {

            case PJSUA_CALL_MEDIA_NONE:
                break;

            case PJSUA_CALL_MEDIA_LOCAL_HOLD:
                break;

            case PJSUA_CALL_MEDIA_ACTIVE:
            case PJSUA_CALL_MEDIA_REMOTE_HOLD: {
                if (!m_isSilent) {
                    RingToneFactory::instance().ringingTone()->stop();

                    qCInfo(lcSIPCall) << "Found media, index" << i << "of" << ci.media.size();
                    aud_med = getAudioMedia(i);
                    mic_media.startTransmit(aud_med);
                    aud_med.startTransmit(speaker_media);

                    auto sniffer = new Sniffer(this);
                    sniffer->initialize();
                    aud_med.startTransmit(dynamic_cast<pj::AudioMediaPort &>(*sniffer));
                    connect(sniffer, &Sniffer::audioLevelChanged, this, [this, sniffer]() {
                        emit SIPCallManager::instance().audioLevelChanged(this,
                                                                          sniffer -> audioLevel());
                    });
                }

                break;
            }
            case PJSUA_CALL_MEDIA_ERROR:
                break;
            }
        }
    }
}

void SIPCall::onInstantMessage(pj::OnInstantMessageParam &prm)
{
    m_imHandler->process(QString::fromStdString(prm.contentType),
                         QString::fromStdString(prm.msgBody));
}

void SIPCall::onInstantMessageStatus(pj::OnInstantMessageStatusParam &prm)
{
    if (prm.code >= 200 && prm.code < 300) {
        return;
    }

    qCWarning(lcSIPCall) << "failed to send message:" << prm.code << prm.reason;
}

pj::AudioMedia *SIPCall::audioMedia() const
{
    const auto callInfo = getInfo();

    for (unsigned i = 0; i < callInfo.media.size(); ++i) {
        if (callInfo.media[i].type == PJMEDIA_TYPE_AUDIO) {
            return static_cast<pj::AudioMedia *>(getMedia(i));
        }
    }
    return nullptr;
}

bool SIPCall::hold()
{
    pj::CallOpParam op(true);

    try {
        setHold(op);
    } catch (pj::Error &err) {
        qCWarning(lcSIPCall) << "failed to hold call:" << err.info();
        return false;
    }

    setIsHolding(true);
    return true;
}

bool SIPCall::unhold()
{
    pj::CallOpParam op(true);
    op.opt.flag = PJSUA_CALL_UNHOLD;

    try {
        reinvite(op);
    } catch (pj::Error &err) {
        qCWarning(lcSIPCall) << "failed to un-hold call:" << err.info();
        return false;
    }

    setIsHolding(false);
    return true;
}

void SIPCall::accept()
{
    if (isEstablished()) {
        qCCritical(lcSIPCall) << "Cannot accept call that is already established";
        return;
    }

    pj::CallOpParam prm;
    prm.statusCode = PJSIP_SC_OK;
    answer(prm);
}

void SIPCall::reject()
{
    if (isEstablished()) {
        qCCritical(lcSIPCall) << "Cannot reject call that is already established";
        return;
    }

    pj::CallOpParam prm;
    prm.statusCode = PJSIP_SC_DECLINE;
    answer(prm);
}

void SIPCall::setIsHolding(bool value)
{
    if (m_isHolding != value) {
        m_isHolding = value;
        m_proxy->setHold(value);
        emit isHoldingChanged();
    }
}

void SIPCall::setIsBlocked(bool value)
{
    if (m_isBlocked != value) {
        m_isBlocked = value;
        emit isBlockedChanged();
    }
}

void SIPCall::setContactInfo(const QString &sipUrl, bool isIncoming)
{
    if (m_sipUrl != sipUrl) {
        m_sipUrl = sipUrl;

        if (m_historyItem && m_wasEstablished) {
            m_historyItem->endCall();
        }

        const auto contactInfo = PhoneNumberUtil::instance().contactInfoBySipUrl(sipUrl);
        m_isEmergencyCall = PhoneNumberUtil::isEmergencyCallUrl(sipUrl);
        m_contactId = contactInfo.contact ? contactInfo.contact->id() : "";

        if (!contactInfo.isAnonymous) {
            NumberStats::instance().incrementCallCount(contactInfo.phoneNumber);
        }

        updateIsBlocked();

        m_historyItem = CallHistory::instance().addHistoryItem(
                isBlocked() ? CallHistoryItem::Type::IncomingBlocked
                            : (isIncoming ? CallHistoryItem::Type::Incoming
                                          : CallHistoryItem::Type::Outgoing),
                m_account->id(), sipUrl, m_contactId, contactInfo.isSipSubscriptable);

        emit contactChanged();
    }
}

void SIPCall::updateIsBlocked()
{
    bool isBlocked = false;

    const auto &callManager = SIPCallManager::instance();

    if (!m_contactId.isEmpty()) {
        isBlocked = callManager.isContactBlocked(m_contactId);
    }

    if (!isBlocked) {
        const auto contactInfo = PhoneNumberUtil::instance().contactInfoBySipUrl(m_sipUrl);

        if (!contactInfo.isAnonymous) {
            isBlocked = callManager.isPhoneNumberBlocked(contactInfo.phoneNumber);
        }
    }

    setIsBlocked(isBlocked);
}

bool SIPCall::hasCapability(const QString &capability) const
{
    return m_imHandler->hasCapability(capability);
}

bool SIPCall::triggerCapability(const QString &capability) const
{
    return m_imHandler->triggerCapability(capability);
}

bool SIPCall::isEmergencyCall() const
{
    return m_isEmergencyCall;
}

void SIPCall::onCallTransferRequest(pj::OnCallTransferRequestParam &prm)
{
    /* Create new Call for call transfer */
    prm.newCall = new SIPCall(m_account);
    // TODO: account calls[]?
}

void SIPCall::onCallReplaceRequest(pj::OnCallReplaceRequestParam &prm)
{
    /* Create new Call for call replace */
    prm.newCall = new SIPCall(m_account);
    // TODO: account calls[]?
}

void SIPCall::createOngoingCallNotification()
{
    pj::CallInfo ci = getInfo();

    // Create notification text
    const auto contactInfo =
            PhoneNumberUtil::instance().contactInfoBySipUrl(QString::fromStdString(ci.remoteUri));
    const Contact *c = contactInfo.contact;
    QStringList bodyParts;

    const QString title =
            tr("Active call with %1")
                    .arg((c && !c->name().isEmpty()) ? c->name() : contactInfo.phoneNumber);

    if (c && !c->company().isEmpty()) {
        bodyParts.append(c->company());
    }

    auto countries = contactInfo.countries;
    if (!contactInfo.city.isEmpty()) {
        countries.push_front(contactInfo.city);
    }
    if (countries.size()) {
        bodyParts.append(countries.join(", "));
    }

    auto n = new Notification(title, bodyParts.join("\n"), Notification::Priority::normal, this);

    auto &am = AvatarManager::instance();
    QString avatar = c ? am.avatarPathFor(c->id()) : "";

    if (avatar.isEmpty()) {
        n->setIcon("call-incoming-symbolic");
    } else {
        n->setIcon(avatar);
        n->setRoundedIcon(true);
    }

    n->setDisplayHint(Notification::tray | Notification::hideContentOnLockScreen);
    n->setCategory("call.ongoing");
    n->addButton(tr("Hang up"), "hangup", "call.hang-up", {});

    QString ref = NotificationManager::instance().add(n);
    connect(n, &Notification::actionInvoked, this, [this, ref](QString action, QVariantList) {
        if (action == "hangup") {
            NotificationManager::instance().remove(ref);

            pj::CallOpParam prm;
            prm.statusCode = PJSIP_SC_OK;
            hangup(prm);
        }
    });
}
