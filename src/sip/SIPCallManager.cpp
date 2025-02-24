#include <QLoggingCategory>
#include <QRegularExpression>

#include "SIPManager.h"
#include "SIPCallManager.h"
#include "SIPAccountManager.h"
#include "SIPAudioManager.h"
#include "ExternalMediaManager.h"
#include "Notification.h"
#include "NotificationManager.h"
#include "Ringer.h"
#include "RingToneFactory.h"
#include "RingTone.h"
#include "PhoneNumberUtil.h"
#include "DtmfGenerator.h"
#include "EnumTranslation.h"
#include "StateManager.h"
#include "ViewHelper.h"
#include "AvatarManager.h"
#include "HeadsetDevices.h"
#include "HeadsetDeviceProxy.h"
#include "AddressBook.h"
#include "BusylightDeviceManager.h"

Q_LOGGING_CATEGORY(lcSIPCallManager, "gonnect.sip.callmanager")

using namespace std::chrono_literals;

SIPCallManager::SIPCallManager(QObject *parent) : QObject(parent)
{
    connect(this, &SIPCallManager::incomingCall, this, &SIPCallManager::onIncomingCall);
    connect(this, &SIPCallManager::incomingCall, this, &SIPCallManager::updateCallCount);
    connect(this, &SIPCallManager::isHoldingChanged, this, &SIPCallManager::updateBusylightState);
    connect(&SIPAudioManager::instance(), &SIPAudioManager::isAudioCaptureMutedChanged, this, &SIPCallManager::updateBusylightState);

    m_dtmfTimer.setInterval(PJSUA_CALL_SEND_DTMF_DURATION_DEFAULT + 10);
    m_dtmfTimer.callOnTimeout(this, &SIPCallManager::dispatchDtmfBuffer);

    // End confernence mode if one participant hangs up
    connect(this, &SIPCallManager::establishedCallsCountChanged, this, [this]() {
        if (m_isConferenceMode && m_establishedCallsCount < 2) {
            m_isConferenceMode = false;
            emit isConferenceModeChanged();
        }
    });

    // React on Headset events
    auto &hds = HeadsetDevices::instance();
    auto dev = hds.getProxy();
    connect(dev, &HeadsetDeviceProxy::hookSwitch, this, [dev, this]() {
        // Were're busy with one call -> end call
        if (!dev->getHookSwitch() && m_calls.count() == 1) {
            auto call = m_calls.first();
            if (call->isEstablished() || !call->isIncoming()) {
                endCall(call);
            }
            return;
        }

        // If we're not busy -> accept call
        if (dev->getHookSwitch() && m_calls.count() == 1) {
            auto call = m_calls.first();
            if (!call->isEstablished() && call->isIncoming()) {
                call->accept();
            }
            return;
        }

        // Idle
        if (dev->getHookSwitch() && m_calls.count() == 0) {
            AppSettings settings;
            if (settings.value("generic/headsetHookOff", true).toBool()) {
                emit ViewHelper::instance().activateSearch();
            }
        }
    });

    m_blockCleanTimer.setInterval(1min);
    m_blockCleanTimer.callOnTimeout(this, &SIPCallManager::cleanupBlocks);

    connect(this, &SIPCallManager::blocksChanged, this, &SIPCallManager::updateBlockTimerRunning);
}

void SIPCallManager::onIncomingCall(SIPCall *call)
{
    pj::CallInfo ci = call->getInfo();

    const bool isEmergency = call->isEmergencyCall();

    initBridge();

    // Send BUSY_HERE if we're on the phone
    if (!isEmergency && hasEstablishedCalls()
        && m_settings.value("generic/busyOnBusy", false).toBool()) {
        pj::CallOpParam prm;
        prm.statusCode = PJSIP_SC_BUSY_HERE;
        call->answer(prm);
        return;
    }

    // Create notification text
    const auto contactInfo =
            PhoneNumberUtil::instance().contactInfoBySipUrl(QString::fromStdString(ci.remoteUri));
    const auto c = contactInfo.contact;
    QStringList bodyParts;
    QString displayName = contactInfo.phoneNumber;
    auto numberType = contactInfo.numberType;

    if (!contactInfo.displayName.isEmpty()) {
        displayName = contactInfo.displayName;

        if (c && numberType == Contact::NumberType::Unknown) {
            numberType = c->phoneNumberObject(contactInfo.phoneNumber).type;
        }
    }

    QString title;
    if (numberType == Contact::NumberType::Unknown) {
        title = tr("%1 is calling").arg(displayName);
    } else {
        title = tr("%1 (%2) is calling")
                        .arg(displayName, EnumTranslation::instance().numberType(numberType));
    }

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

    // Create notification object
    if (call->isBlocked()) {
        qCInfo(lcSIPCallManager) << "Incoming call from" << displayName << contactInfo.phoneNumber
                                 << "has been blocked";
        return;
    }

    auto n = new Notification(title, bodyParts.join("\n"), Notification::Priority::urgent, call);

    if (m_settings.value("generic/inverseAcceptReject", false).toBool()) {
        n->addButton(tr("Reject"), "reject", "call.decline", {});
        n->addButton(tr("Accept"), "accept", "call.accept", {});
    } else {
        n->addButton(tr("Accept"), "accept", "call.accept", {});
        n->addButton(tr("Reject"), "reject", "call.decline", {});
    }

    auto &am = AvatarManager::instance();
    QString avatar = c ? am.avatarPathFor(c->id()) : "";

    if (avatar.isEmpty()) {
        n->setIcon("call-incoming-symbolic");
    } else {
        n->setIcon(avatar);
        n->setRoundedIcon(true);
        n->setEmblem("call-incoming");
    }
    n->setDefaultAction("show");
    n->setCategory("call.incoming");
    n->setDisplayHint(Notification::tray | Notification::hideContentOnLockScreen);

    // If we've no call ongoing, pause external media - even if we're just ringing the bell
    if (!isEmergency && hasEstablishedCalls()) {
        RingToneFactory::instance().zipTone()->start();
    } else {
        ExternalMediaManager::instance().pause();
        StateManager::instance().inhibitScreenSaver();

        auto ringer = new Ringer(n);
        ringer->start();

        BusylightDeviceManager::instance().startBlinking(Qt::GlobalColor::green);
        connect(n, &QObject::destroyed, this,
                []() { BusylightDeviceManager::instance().stopBlinking(); });

        pj::CallOpParam prm;
        prm.statusCode = PJSIP_SC_RINGING;
        call->answer(prm);
    }

    connect(n, &Notification::actionInvoked, call, [this, call](QString action, QVariantList) {
        if (action == "accept") {
            acceptCall(call);
        } else if (action == "reject") {
            rejectCall(call);
        } else {
            emit showCallWindow();
        }
    });

    QString ref = NotificationManager::instance().add(n);
    call->setNotificationRef(ref);
    connect(call, &SIPCall::destroyed, this,
            [ref]() { NotificationManager::instance().remove(ref); });
    connect(call, &SIPCall::establishedChanged, this,
            [ref]() { NotificationManager::instance().remove(ref); });

    if (isEmergency) {
        emit ViewHelper::instance().showEmergency(call -> account()->id(), call->getId(),
                                                  displayName);
    }
}

void SIPCallManager::updateCallCount()
{
    quint8 establishedCallsCount = 0;
    bool earlyMediaActive = false;

    if (m_calls.length() != m_activeCalls) {
        m_activeCalls = m_calls.length();
        emit activeCallsChanged();
    }

    if (m_activeCalls == 0) {
        ExternalMediaManager::instance().resume();
        StateManager::instance().releaseScreenSaver();
    } else {
        for (auto call : std::as_const(m_calls)) {
            if (call->isEstablished()) {
                ++establishedCallsCount;
            }
            if (call->earlyMediaActive()) {
                earlyMediaActive = true;
            }
        }
    }

    if (m_activeCalls <= establishedCallsCount) {
        RingToneFactory::instance().zipTone()->stop();
    }

    if (establishedCallsCount != m_establishedCallsCount) {
        m_establishedCallsCount = establishedCallsCount;
        emit establishedCallsCountChanged();
    }

    if ((establishedCallsCount > 0) != m_hasEstablishedCalls) {
        m_hasEstablishedCalls = establishedCallsCount > 0;
        emit hasEstablishedCallsChanged();
    }

    if (m_earlyMediaActive != earlyMediaActive) {
        m_earlyMediaActive = earlyMediaActive;
        emit earlyMediaActiveChanged();
    }
}

void SIPCallManager::call(const QString &number, bool silent)
{
    const auto &accounts = SIPAccountManager::instance().accounts();
    if (!accounts.isEmpty()) {
        const auto phoneNumber = PhoneNumberUtil::isSipUri(number)
                ? number
                : PhoneNumberUtil::cleanPhoneNumber(number);
        call(accounts.first()->id(), phoneNumber, "", "", silent);
    } else {
        qCCritical(lcSIPCallManager) << "no accounts to start call with";
    }
}

void SIPCallManager::initBridge()
{
    if (!m_bridgeConfigured) {
        auto &audDevManager = SIPManager::instance().endpoint().audDevManager();
        audDevManager.setNullDev();

        m_bridgeConfigured = true;
    }
}

void SIPCallManager::call(const QString &accountId, const QString &number, const QString &contactId,
                          const QString &preferredIdentity, bool silent)
{

    initBridge();

    // Check if there is already a call for that target number
    for (auto call : std::as_const(m_calls)) {
        auto callRemoteNumber = PhoneNumberUtil::numberFromSipUrl(
                QString::fromStdString(call->getInfo().remoteUri));
        if (number == callRemoteNumber) {
            qCInfo(lcSIPCallManager) << "skipping additional call to already connected URI"
                                     << call->getInfo().remoteUri;
            return;
        }
    }

    // Hold other calls - we only allow one outgoing call to be active at the time of dialing
    if (!silent) {
        holdAllCalls();
    }

    auto account = SIPAccountManager::instance().getAccount(accountId);

    if (account) {
        if (!silent) {
            ExternalMediaManager::instance().pause();
            StateManager::instance().inhibitScreenSaver();
        }

        account->call(number, contactId, preferredIdentity, silent);
    } else {
        qCCritical(lcSIPCallManager) << "not starting call - unknown account:" << accountId;
    }
}

void SIPCallManager::endAllCalls()
{
    for (auto call : std::as_const(m_calls)) {
        call->account()->hangup(call->getId());
    }
}

void SIPCallManager::holdOtherCalls(const SIPCall *call)
{
    for (auto otherCall : std::as_const(m_calls)) {
        if (otherCall != call) {
            otherCall->hold();
        }
    }
}

void SIPCallManager::holdAllCalls() const
{
    for (auto call : std::as_const(m_calls)) {
        call->hold();
    }
}

void SIPCallManager::unholdAllCalls() const
{
    for (auto call : std::as_const(m_calls)) {
        call->unhold();
    }
}

void SIPCallManager::endCall(const QString &accountId, const int callId)
{
    auto account = SIPAccountManager::instance().getAccount(accountId);

    if (account) {
        account->hangup(callId);
    } else {
        qCCritical(lcSIPCallManager) << "Account with id" << accountId << "not found";
    }
}

void SIPCallManager::endCall(SIPCall *call)
{
    if (call) {
        call->account()->hangup(call->getId());
    }
}

void SIPCallManager::holdCall(const QString &accountId, const int callId)
{
    auto account = SIPAccountManager::instance().getAccount(accountId);

    if (account) {
        account->hold(callId);
    } else {
        qCCritical(lcSIPCallManager) << "Account with id" << accountId << "not found";
    }
}

void SIPCallManager::unholdCall(const QString &accountId, const int callId)
{
    auto account = SIPAccountManager::instance().getAccount(accountId);

    if (account) {
        account->unhold(callId);
    } else {
        qCCritical(lcSIPCallManager) << "Account with id" << accountId << "not found";
    }
}

void SIPCallManager::acceptCall(const QString &accountId, const int callId)
{
    auto account = SIPAccountManager::instance().getAccount(accountId);

    if (account) {
        if (auto call = account->getCallById(callId)) {
            acceptCall(call);
        }
    } else {
        qCCritical(lcSIPCallManager) << "Account with id" << accountId << "not found";
    }
}

void SIPCallManager::acceptCall(SIPCall *call)
{
    Q_CHECK_PTR(call);

    if (hasActiveCalls()) {
        RingToneFactory::instance().zipTone()->stop();
    }

    if (call->isEmergencyCall()) {
        terminateAllNonEmergencyCalls();
    } else {
        holdOtherCalls(call);
    }

    const auto ref = call->notificationRef();
    if (!ref.isEmpty()) {
        NotificationManager::instance().remove(ref);
    }

    call->accept();
}

void SIPCallManager::rejectCall(SIPCall *call)
{
    Q_CHECK_PTR(call);

    if (hasActiveCalls()) {
        RingToneFactory::instance().zipTone()->stop();
    }

    const auto ref = call->notificationRef();
    if (!ref.isEmpty()) {
        NotificationManager::instance().remove(ref);
    }
    call->reject();
}

void SIPCallManager::terminateAllNonEmergencyCalls()
{
    QList<SIPCall *> callsToTerminate;

    for (auto activeCall : std::as_const(m_calls)) {
        if (!activeCall->isEmergencyCall()) {
            callsToTerminate.append(activeCall);
        }
    }

    for (auto call : std::as_const(callsToTerminate)) {
        endCall(call);
    }
}

void SIPCallManager::transferCall(const QString &fromAccountId, int fromCallId,
                                  const QString &toAccountId, int toCallId)
{
    const auto fromCall = findCall(fromAccountId, fromCallId);
    if (!fromCall) {
        qCCritical(lcSIPCallManager)
                << "Cannot find call" << fromCallId << "in account" << fromAccountId;
        return;
    }
    auto toCall = findCall(toAccountId, toCallId);
    if (!toCall) {
        qCCritical(lcSIPCallManager)
                << "Cannot find call" << toAccountId << "in account" << toCallId;
        return;
    }

    toCall->xferReplaces(*fromCall, pj::CallOpParam());
    endCall(toCall);
}

SIPCall *SIPCallManager::findCall(const QString &accountId, int callId) const
{
    for (auto call : std::as_const(m_calls)) {
        if (call->getId() == callId && call->account()->id() == accountId) {
            return call;
        }
    }
    return nullptr;
}

SIPCall *SIPCallManager::findCall(const QString &remoteUri) const
{
    if (!remoteUri.isEmpty()) {
        for (auto call : std::as_const(m_calls)) {
            if (QString::fromStdString(call->getInfo().remoteUri) == remoteUri) {
                return call;
            }
        }
    }
    return nullptr;
}

void SIPCallManager::triggerCapability(const QString &accountId, const int callId,
                                       const QString &capability) const
{
    if (auto call = findCall(accountId, callId)) {
        call->triggerCapability(capability);
        return;
    }

    qCCritical(lcSIPCallManager) << "Unable to find account with id" << accountId
                                 << "and call with id" << callId;
}

void SIPCallManager::startConference()
{

    if (m_establishedCallsCount != 2) {
        qCCritical(lcSIPCallManager)
                << "Can only make conference with two established calls, but got"
                << m_establishedCallsCount;
        return;
    }

    if (!m_isConferenceMode) {
        Q_ASSERT(m_calls.size() == 2);

        unholdAllCalls();

        QTimer::singleShot(100, this, [this]() {
            pj::AudioMedia *audioMedia1 = q_check_ptr(m_calls.at(0)->audioMedia());
            pj::AudioMedia *audioMedia2 = q_check_ptr(m_calls.at(1)->audioMedia());

            try {
                audioMedia1->startTransmit(*audioMedia2);
            } catch (pj::Error err) {
                qCCritical(lcSIPCallManager) << "Error transmitting audioMedia1 to audioMedia2:\n"
                                             << "  status:" << err.status << "\n"
                                             << "  reason:" << err.reason << "\n"
                                             << "  file and line:" << err.srcFile << err.srcLine;
            }
            try {
                audioMedia2->startTransmit(*audioMedia1);
            } catch (pj::Error err) {
                qCCritical(lcSIPCallManager)
                        << "Error transmitting audioMedia2 to audioMedia1:\n"
                        << "  status:" << err.status << "\n"
                        << "  reason:" << err.reason << "\n"
                        << "  file and line:" << err.srcFile << err.srcLine << "\n";
            }

            m_isConferenceMode = true;
            emit isConferenceModeChanged();
        });
    } else {
        qCWarning(lcSIPCallManager) << "Already in conference mode";
    }
}

void SIPCallManager::endConference()
{
    if (m_isConferenceMode) {
        endAllCalls();
        m_isConferenceMode = false;
        emit isConferenceModeChanged();
    } else {
        qCWarning(lcSIPCallManager) << "Not in conference mode";
    }
}

void SIPCallManager::toggleHold()
{
    if (m_hasEstablishedCalls && m_calls.count() == 1) {
        auto call = m_calls.first();
        if (call->isHolding()) {
            call->unhold();
        } else {
            call->hold();
        }
    }
}

bool SIPCallManager::isOneCallOnHold() const
{
    for (const auto call : std::as_const(m_calls)) {
        if (call->isHolding()) {
            return true;
        }
    }
    return false;
}

void SIPCallManager::addCall(SIPCall *call)
{
    m_calls.push_back(call);
    connect(call, &SIPCall::earlyMediaActiveChanged, this, &SIPCallManager::updateCallCount);
    connect(call, &SIPCall::establishedChanged, this, &SIPCallManager::updateCallCount);
    connect(call, &SIPCall::establishedChanged, this,
            [call, this]() { emit establishedChanged(call); });
    connect(call, &SIPCall::isHoldingChanged, this,
            [call, this]() { emit isHoldingChanged(call); });
    connect(call, &SIPCall::capabilitiesChanged, this,
            [call, this]() { emit capabilitiesChanged(call); });
    connect(call, &SIPCall::contactChanged, this,
            [call, this]() { emit callContactChanged(call); });

    connect(call, &SIPCall::missed, this, [this, call]() {
        if (call->isBlocked()) {
            return;
        }

        pj::CallInfo ci = call->getInfo();

        m_missedCalls++;
        emit missedCallsChanged();

        // Create notification text
        const auto contactInfo = PhoneNumberUtil::instance().contactInfoBySipUrl(
                QString::fromStdString(ci.remoteUri));
        const Contact *c = contactInfo.contact;
        QStringList bodyParts;

        const QString title =
                tr("Missed call from %1")
                        .arg((c && !c->name().isEmpty()) ? c->name() : contactInfo.phoneNumber);
        const QString number = contactInfo.phoneNumber;

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

        QPointer<SIPAccount> account = call->account();

        auto n =
                new Notification(title, bodyParts.join("\n"), Notification::Priority::normal, this);

        auto &am = AvatarManager::instance();
        QString avatar = c ? am.avatarPathFor(c->id()) : "";

        if (avatar.isEmpty()) {
            n->setIcon("call-missed-symbolic");
        } else {
            n->setIcon(avatar);
            n->setRoundedIcon(true);
            n->setEmblem("call-missed");
        }

        n->setDisplayHint(Notification::tray | Notification::hideContentOnLockScreen);
        n->setCategory("call.unanswered");
        n->addButton(tr("Call back"), "call", "call.accept", {});

        QString ref = NotificationManager::instance().add(n);

        connect(n, &Notification::actionInvoked, this,
                [c, number, account, ref](QString, QVariantList) {
                    if (account) {
                        account->call(number, c ? c->id() : "");
                    }

                    NotificationManager::instance().remove(ref);
                });
    });

    emit callsChanged();
}

void SIPCallManager::removeCall(SIPCall *call)
{
    m_calls.removeAll(call);
    emit callsChanged();

    updateCallCount();
}

void SIPCallManager::resetMissedCalls()
{
    m_missedCalls = 0;
    emit missedCallsChanged();
}

void SIPCallManager::toggleTemporaryBlock(const QString &contactId, const QString &phoneNumber)
{

    const auto &addressBook = AddressBook::instance();
    const QDateTime inEightHours = QDateTime::currentDateTime().addSecs(8 * 60 * 60);

    bool contactFound = false;
    bool wasAdded = false;
    bool wasRemoved = false;

    // Toggle by contact id
    if (!contactId.isEmpty()) {
        const auto contact = addressBook.lookupByContactId(contactId);
        if (contact) {
            contactFound = true;

            for (int i = 0; i < m_tempBlockedContacts.size(); ++i) {
                if (m_tempBlockedContacts.at(i).second == contactId) {
                    m_tempBlockedContacts.removeAt(i);
                    wasRemoved = true;
                    break;
                }
            }

            if (!wasRemoved) {
                m_tempBlockedContacts.append(qMakePair(inEightHours, contactId));
                wasAdded = true;
            }
        }
    }

    // Toggle by phone number
    if (!wasAdded && !wasRemoved && !contactFound && !phoneNumber.isEmpty()) {
        for (int i = 0; i < m_tempBlockedNumbers.size(); ++i) {
            if (m_tempBlockedNumbers.at(i).second == phoneNumber) {
                wasRemoved = true;
                m_tempBlockedNumbers.removeAt(i);
                break;
            }
        }

        if (!wasRemoved) {
            m_tempBlockedNumbers.append(qMakePair(inEightHours, phoneNumber));
            wasAdded = true;
        }
    }

    if (wasAdded || wasRemoved) {
        emit blocksChanged();
    }
}

bool SIPCallManager::isContactBlocked(const QString &contactId) const
{
    if (contactId.isEmpty()) {
        return false;
    }

    for (const auto &pair : std::as_const(m_tempBlockedContacts)) {
        if (pair.second == contactId) {
            return true;
        }
    }
    return false;
}

bool SIPCallManager::isPhoneNumberBlocked(const QString &phoneNumber) const
{
    if (phoneNumber.isEmpty()) {
        return false;
    }

    for (const auto &pair : std::as_const(m_tempBlockedNumbers)) {
        if (pair.second == phoneNumber) {
            return true;
        }
    }

    return false;
}

void SIPCallManager::sendDtmf(const QString &accountId, const int callId, const QString &digit)
{

    const auto bufferKey = std::make_pair(accountId, callId);
    const QString old = m_dtmfBuffer.value(bufferKey, "");
    m_dtmfBuffer.insert(bufferKey, old + digit);

    if (!m_dtmfTimer.isActive()) {
        m_dtmfTimer.start();
    }
}

void SIPCallManager::dispatchDtmfBuffer()
{

    QMutableHashIterator it(m_dtmfBuffer);
    while (it.hasNext()) {
        it.next();

        const auto accountId = it.key().first;
        if (const auto account = SIPAccountManager::instance().getAccount(accountId)) {
            const auto callId = it.key().second;

            const auto calls = account->calls();
            for (auto call : calls) {
                if (call && call->getId() == callId) {
                    const auto &val = it.value();
                    bool abortAfter = false;

                    if (val.front() == QChar(',')) {
                        m_dtmfTimer.setInterval(500);
                        m_dtmfTimer.start();
                        abortAfter = true;

                    } else {
                        // Call found - consume one digit from string and send as DTMF
                        if (!m_dtmfGen) {
                            m_dtmfGen = new DtmfGenerator(this);
                        }

                        m_dtmfGen->playDtmf(val.front());
                        call->dialDtmf(val.first(1).toStdString());

                        m_dtmfTimer.setInterval(PJSUA_CALL_SEND_DTMF_DURATION_DEFAULT + 10);
                    }

                    const auto remainingString = val.mid(1);
                    if (remainingString.isEmpty()) {
                        it.remove();
                    } else {
                        it.setValue(remainingString);
                    }

                    if (abortAfter) {
                        return;
                    } else {
                        break;
                    }
                }
            }
        } else {
            qCCritical(lcSIPCallManager) << "Account with id" << accountId << "not found";
        }
    }

    if (m_dtmfBuffer.size() == 0) {
        m_dtmfTimer.stop();
    }
}

void SIPCallManager::cleanupBlocks()
{
    bool anythingRemoved = false;
    const auto now = QDateTime::currentDateTime();

    QMutableListIterator it(m_tempBlockedContacts);
    while (it.hasNext()) {
        const auto &pair = it.next();
        if (pair.first < now) {
            anythingRemoved = true;
            it.remove();
        }
    }

    QMutableListIterator itNum(m_tempBlockedNumbers);
    while (itNum.hasNext()) {
        const auto &pair = itNum.next();
        if (pair.first < now) {
            anythingRemoved = true;
            itNum.remove();
        }
    }

    if (anythingRemoved) {
        emit blocksChanged();
    }
}

void SIPCallManager::updateBlockTimerRunning()
{
    if (m_blockCleanTimer.isActive() && m_tempBlockedContacts.size() == 0
        && m_tempBlockedNumbers.size() == 0) {
        m_blockCleanTimer.stop();
    } else if (!m_blockCleanTimer.isActive()
               && (m_tempBlockedContacts.size() || m_tempBlockedNumbers.size())) {
        m_blockCleanTimer.start();
    }
}

void SIPCallManager::updateBusylightState()
{
    auto& busylightDevManager = BusylightDeviceManager::instance();

    QColor color(Qt::GlobalColor::red);
    if (SIPAudioManager::instance().isAudioCaptureMuted()) {
        color.setRgb(255, 165, 0);
    }

    if (isOneCallOnHold()) {
        busylightDevManager.startBlinking(color);
    } else {
        busylightDevManager.switchOn(color);
    }
}
