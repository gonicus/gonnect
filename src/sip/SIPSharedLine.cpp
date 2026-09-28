#include <QLoggingCategory>
#include "SIPSharedLine.h"
#include "SIPAccount.h"
#include "PjSipTools.h"

#include <pjsua-lib/pjsua.h>

Q_LOGGING_CATEGORY(lcSIPSharedLine, "gonnect.sip.sharedline")

SIPSharedLine::SIPSharedLine(SIPAccount *account, const QString &uri)
    : QObject(account), pj::Buddy(), m_uri(uri), m_account(account)
{
}

SIPSharedLine::~SIPSharedLine() { }

bool SIPSharedLine::initialize()
{
    pj::BuddyConfig cfg;
    cfg.uri = m_uri.toStdString();

    // Only go for dialog events here
    cfg.subscribe = false;
    cfg.subscribe_dlg_event = true;

    try {
        create(*m_account, cfg);
    } catch (const pj::Error &err) {
        qCCritical(lcSIPSharedLine)
                << "failed to create shared line watcher for" << m_uri << ":" << err.info(false);
        return false;
    }

    setSubscriptionState(SubscriptionState::Pending);
    qCInfo(lcSIPSharedLine) << "shared line watcher for " << m_uri << "created";

    return true;
}

void SIPSharedLine::setSubscriptionState(SubscriptionState state)
{
    if (m_subscriptionState == state) {
        return;
    }

    m_subscriptionState = state;
    Q_EMIT subscriptionStateChanged();
}

void SIPSharedLine::clearRemoteDialog()
{
    m_remoteCallId.clear();
    m_remoteLocalTag.clear();
    m_remoteRemoteTag.clear();
    m_remoteState.clear();
    m_remoteDirection.clear();

    if (m_remoteInUse) {
        m_remoteInUse = false;
        Q_EMIT remoteInUseChanged();
    }
}

void SIPSharedLine::onBuddyEvSubDlgEventState(pj::OnBuddyEvSubStateParam &prm)
{
    if (prm.e.type != PJSIP_EVENT_TSX_STATE) {
        return;
    }

    const int code = prm.e.body.tsxState.tsx.statusCode;

    // Ignore typical errors that point to "no shared line available"
    if (code == PJSIP_SC_BAD_EVENT || code == PJSIP_SC_METHOD_NOT_ALLOWED) {
        qCInfo(lcSIPSharedLine) << "no shared line configured for" << m_uri;
        clearRemoteDialog();
        setSubscriptionState(SubscriptionState::NotConfigured);
        return;
    }

    if (code >= 200 && code < 300) {
        setSubscriptionState(SubscriptionState::Active);
        return;
    }

    if (code >= 300) {
        qCWarning(lcSIPSharedLine) << "dialog event subscription for" << m_uri << "failed:" << code
                                   << QString::fromStdString(prm.e.body.tsxState.tsx.statusText);
        clearRemoteDialog();
        setSubscriptionState(SubscriptionState::Failed);
    }
}

void SIPSharedLine::onBuddyDlgEventState()
{
    // Get dialog info event info from C layer
    pjsua_buddy_dlg_event_info info;
    pj_bzero(&info, sizeof(info));

    const pj_status_t status = pjsua_buddy_get_dlg_event_info(getId(), &info);
    if (status != PJ_SUCCESS) {
        qCWarning(lcSIPSharedLine) << "failed to read dialog event info for" << m_uri;
        return;
    }

    const QString dialogState = PjSipTools::pjStringToQstring(info.dialog_state);
    const QString callId = PjSipTools::pjStringToQstring(info.dialog_call_id);

    qCDebug(lcSIPSharedLine).nospace()
            << "dialog event for " << m_uri << ": state=" << dialogState << " call-id=" << callId
            << " local-tag=" << PjSipTools::pjStringToQstring(info.dialog_local_tag)
            << " remote-tag=" << PjSipTools::pjStringToQstring(info.dialog_remote_tag)
            << " direction=" << PjSipTools::pjStringToQstring(info.dialog_direction);

    // Only take "confirmed"
    const bool inUse =
            dialogState.compare("confirmed", Qt::CaseInsensitive) == 0 && !callId.isEmpty();

    if (!inUse) {
        clearRemoteDialog();
        return;
    }

    m_remoteCallId = callId;
    m_remoteLocalTag = PjSipTools::pjStringToQstring(info.dialog_local_tag);
    m_remoteRemoteTag = PjSipTools::pjStringToQstring(info.dialog_remote_tag);
    m_remoteState = dialogState;
    m_remoteDirection = PjSipTools::pjStringToQstring(info.dialog_direction);

    if (!m_remoteInUse) {
        m_remoteInUse = true;
        qCInfo(lcSIPSharedLine) << "shared line" << m_uri << "is active on another device";
        Q_EMIT remoteInUseChanged();
    }
}

QString SIPSharedLine::joinHeaderValue() const
{
    if (!m_remoteInUse || m_remoteCallId.isEmpty() || m_remoteLocalTag.isEmpty()
        || m_remoteRemoteTag.isEmpty()) {
        return {};
    }

    // The tags have to be swapped due to different viewpoints: observer (RFC 4235)
    // PBX (RFC 3911)
    return QString("%1;to-tag=%2;from-tag=%3")
            .arg(m_remoteCallId, m_remoteRemoteTag, m_remoteLocalTag);
}
