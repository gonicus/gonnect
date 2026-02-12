#include "CallsModel.h"
#include "SIPCallManager.h"
#include "SIPAccount.h"
#include "PhoneNumberUtil.h"
#include "Application.h"

// --- DEV ---
#include "AddressBook.h"
#include "Contact.h"
// --- DEV ---

#include <QRegularExpression>
#include <QTimer>
#include <QtAudio>

CallsModel::CallsModel(QObject *parent) : QAbstractListModel{ parent }
{

    auto &callManager = SIPCallManager::instance();
    connect(&callManager, &SIPCallManager::establishedChanged, this, [this](SIPCall *call) {
        auto callInfo = m_callsHash.value(call->getId());
        const auto index = m_calls.indexOf(callInfo);
        if (index >= 0) {
            callInfo->isEstablished = call->isEstablished();
            callInfo->established = call->establishedTime();
            callInfo->hasCapabilityJitsi = call->hasCapability("jitsi") && callInfo->isEstablished;

            auto idx = createIndex(index, 0);
            Q_EMIT dataChanged(idx, idx,
                               {
                                       static_cast<int>(Roles::IsEstablished),
                                       static_cast<int>(Roles::EstablishedTime),
                                       static_cast<int>(Roles::HasCapabilityJitsi),
                               });
        }
    });

    connect(&callManager, &SIPCallManager::metadataChanged, this, [this](SIPCall *call) {
        auto callInfo = m_callsHash.value(call->getId());
        const auto index = m_calls.indexOf(callInfo);
        if (index >= 0) {
            callInfo->hasMetadata = call->hasMetadata();

            auto idx = createIndex(index, 0);
            Q_EMIT dataChanged(idx, idx, { static_cast<int>(Roles::HasMetadata) });
        }
    });

    connect(&callManager, &SIPCallManager::isHoldingChanged, this, [this](SIPCall *call) {
        auto callInfo = m_callsHash.value(call->getId());
        const auto index = m_calls.indexOf(callInfo);

        if (index >= 0) {
            callInfo->isHolding = call->isHolding();

            auto idx = createIndex(index, 0);
            Q_EMIT dataChanged(idx, idx, { static_cast<int>(Roles::IsHolding) });
        }
    });

    connect(&callManager, &SIPCallManager::isBlockedChanged, this, [this](SIPCall *call) {
        auto callInfo = m_callsHash.value(call->getId());
        const auto index = m_calls.indexOf(callInfo);

        if (index >= 0) {
            callInfo->isBlocked = call->isBlocked();

            auto idx = createIndex(index, 0);
            Q_EMIT dataChanged(idx, idx, { static_cast<int>(Roles::IsBlocked) });
        }
    });

    connect(&callManager, &SIPCallManager::audioLevelChanged, this,
            [this](SIPCall *call, qreal level) {
                auto callInfo = m_callsHash.value(call->getId());
                const auto index = m_calls.indexOf(callInfo);

                if (index >= 0) {
                    callInfo->incomingAudioLevel = QtAudio::convertVolume(
                            level, QtAudio::LinearVolumeScale, QtAudio::LogarithmicVolumeScale);

                    auto idx = createIndex(index, 0);
                    Q_EMIT dataChanged(idx, idx,
                                       { static_cast<int>(Roles::HasIncomingAudioLevel) });
                }
            });

    connect(&callManager, &SIPCallManager::qualityLevelChanged, this,
            [this](SIPCall *call, SIPCallManager::QualityLevel qualityLevel) {
                auto callInfo = m_callsHash.value(call->getId());
                const auto index = m_calls.indexOf(callInfo);

                if (callInfo && index >= 0) {
                    callInfo->qualityLevel = qualityLevel;
                    const auto idx = createIndex(index, 0);
                    Q_EMIT dataChanged(idx, idx, { static_cast<int>(Roles::QualityLevel) });
                }
            });

    connect(&callManager, &SIPCallManager::securityLevelChanged, this,
            [this](SIPCall *call, SIPCallManager::SecurityLevel securityLevel) {
                auto callInfo = m_callsHash.value(call->getId());
                const auto index = m_calls.indexOf(callInfo);

                if (callInfo && index >= 0) {
                    callInfo->securityLevel = securityLevel;
                    const auto idx = createIndex(index, 0);
                    Q_EMIT dataChanged(idx, idx, { static_cast<int>(Roles::SecurityLevel) });
                }
            });

    connect(&callManager, &SIPCallManager::callState, this, [this](int callId, int statusCode) {
        auto callInfo = m_callsHash.value(callId);
        const auto index = m_calls.indexOf(callInfo);

        if (index >= 0) {
            callInfo->statusCode = static_cast<pjsip_status_code>(statusCode);

            auto idx = createIndex(index, 0);
            Q_EMIT dataChanged(idx, idx, { static_cast<int>(Roles::StatusCode) });
        }
    });

    connect(&callManager, &SIPCallManager::capabilitiesChanged, this, [this](SIPCall *call) {
        auto callInfo = m_callsHash.value(call->getId());
        const auto index = m_calls.indexOf(callInfo);

        if (index >= 0) {
            callInfo->hasCapabilityJitsi = call->hasCapability("jitsi") && callInfo->isEstablished;

            auto idx = createIndex(index, 0);
            Q_EMIT dataChanged(idx, idx, { static_cast<int>(Roles::HasCapabilityJitsi) });
        }
    });

    connect(&callManager, &SIPCallManager::activeCallsChanged, this, &CallsModel::updateCalls);
    connect(&callManager, &SIPCallManager::callContactChanged, this, &CallsModel::updateCalls);
    connect(&AddressBook::instance(), &AddressBook::contactsReady, this, &CallsModel::updateCalls);
    updateCalls();
}

CallsModel::~CallsModel()
{
    m_callsHash.clear();
    qDeleteAll(m_calls);
    m_calls.clear();
}

int CallsModel::unfinishedCount() const
{
    int result = 0;
    for (const auto *call : std::as_const(m_calls)) {
        if (!call->isFinished) {
            ++result;
        }
    }
    return result;
}

QHash<int, QByteArray> CallsModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::CallId), "callId" },
        { static_cast<int>(Roles::AccountId), "accountId" },
        { static_cast<int>(Roles::RemoteUri), "remoteUri" },
        { static_cast<int>(Roles::PhoneNumber), "phoneNumber" },
        { static_cast<int>(Roles::IsIncoming), "isIncoming" },
        { static_cast<int>(Roles::ContactName), "contactName" },
        { static_cast<int>(Roles::City), "city" },
        { static_cast<int>(Roles::Country), "country" },
        { static_cast<int>(Roles::Company), "company" },
        { static_cast<int>(Roles::IsEstablished), "isEstablished" },
        { static_cast<int>(Roles::EstablishedTime), "establishedTime" },
        { static_cast<int>(Roles::IsHolding), "isHolding" },
        { static_cast<int>(Roles::IsBlocked), "isBlocked" },
        { static_cast<int>(Roles::StatusCode), "statusCode" },
        { static_cast<int>(Roles::IsFinished), "isFinished" },
        { static_cast<int>(Roles::HasCapabilityJitsi), "hasCapabilityJitsi" },
        { static_cast<int>(Roles::HasIncomingAudioLevel), "hasIncomingAudioLevel" },
        { static_cast<int>(Roles::HasMetadata), "hasMetadata" },
        { static_cast<int>(Roles::HasAvatar), "hasAvatar" },
        { static_cast<int>(Roles::AvatarPath), "avatarPath" },
        { static_cast<int>(Roles::QualityLevel), "qualityLevel" },
        { static_cast<int>(Roles::SecurityLevel), "securityLevel" },
    };
}

void CallsModel::updateCalls()
{

    const auto oldCount = m_calls.size();

    beginResetModel();
    const auto calls = SIPCallManager::instance().calls();

    // Add/update calls
    auto removedCallIds = m_callsHash.keys();

    for (const auto call : calls) {
        if (call->isSilent()) {
            continue;
        }

        const auto callId = call->getId();
        CallInfo *callInfo = nullptr;
        bool exists = false;

        if (m_callsHash.contains(callId)) {
            callInfo = m_callsHash.value(callId);
            exists = true;
        } else {
            callInfo = new CallInfo;
        }

        callInfo->callId = callId;
        callInfo->accountId = qobject_cast<SIPAccount *>(call->parent())->id();
        callInfo->remoteUri = call->sipUrl();
        callInfo->established = call->establishedTime();
        callInfo->isEstablished = call->isEstablished();
        callInfo->isIncoming = call->isIncoming();
        callInfo->isBlocked = call->isBlocked();
        callInfo->isHolding = call->isHolding();
        callInfo->contactInfo =
                PhoneNumberUtil::instance().contactInfoBySipUrl(callInfo->remoteUri);
        callInfo->hasCapabilityJitsi = call->hasCapability("jitsi") && call->isEstablished();
        callInfo->hasMetadata = call->hasMetadata();
        callInfo->qualityLevel = call->qualityLevel();
        callInfo->securityLevel = call->securityLevel();

        if (!exists) {
            m_calls.append(callInfo);
            m_callsHash.insert(callId, callInfo);
        }

        removedCallIds.removeOne(callId);
    }

    // Kill timer for removed calls
    for (const int callId : std::as_const(removedCallIds)) {
        QTimer::singleShot(GONNECT_CALL_VISIBLE_AFTER_END, this, [this, callId]() {
            auto callInfo = m_callsHash.value(callId);
            if (callInfo) {
                const auto idx = m_calls.indexOf(callInfo);
                beginRemoveRows(QModelIndex(), idx, idx);
                m_callsHash.remove(callId);
                m_calls.removeAt(idx);
                delete callInfo;
                endRemoveRows();

                Q_EMIT countChanged();
            }
        });

        auto callInfo = m_callsHash.value(callId);
        const auto idx = m_calls.indexOf(callInfo);
        auto modelIndex = createIndex(idx, 0);
        callInfo->isFinished = true;
        callInfo->hasCapabilityJitsi = false;
        Q_EMIT dataChanged(modelIndex, modelIndex,
                           {
                                   static_cast<int>(Roles::IsFinished),
                                   static_cast<int>(Roles::HasCapabilityJitsi),
                           });
    }

    endResetModel();

    if (oldCount != m_calls.size()) {
        Q_EMIT countChanged();
    }
    Q_EMIT unfinishedCountChanged();
}

int CallsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_calls.size();
}

QVariant CallsModel::data(const QModelIndex &index, int role) const
{
    const auto callInfo = m_calls.at(index.row());

    switch (role) {
    case static_cast<int>(Roles::CallId):
        return callInfo->callId;

    case static_cast<int>(Roles::IsEstablished):
        return callInfo->isEstablished;

    case static_cast<int>(Roles::IsFinished):
        return callInfo->isFinished;

    case static_cast<int>(Roles::EstablishedTime):
        return callInfo->established;

    case static_cast<int>(Roles::IsIncoming):
        return callInfo->isIncoming;

    case static_cast<int>(Roles::IsHolding):
        return callInfo->isHolding;

    case static_cast<int>(Roles::IsBlocked):
        return callInfo->isBlocked;

    case static_cast<int>(Roles::AccountId):
        return callInfo->accountId;

    case static_cast<int>(Roles::StatusCode):
        return callInfo->statusCode;

    case static_cast<int>(Roles::PhoneNumber):
        return callInfo->contactInfo.isAnonymous ? tr("unknown number")
                                                 : callInfo->contactInfo.phoneNumber;

    case static_cast<int>(Roles::ContactName):
        return !callInfo->contactInfo.displayName.isEmpty() ? callInfo->contactInfo.displayName
                                                            : "";

    case static_cast<int>(Roles::City):
        return callInfo->contactInfo.city;

    case static_cast<int>(Roles::Country):
        return callInfo->contactInfo.countries.join(", ");

    case static_cast<int>(Roles::Company):
        return callInfo->contactInfo.contact ? callInfo->contactInfo.contact->company() : "";

    case static_cast<int>(Roles::HasCapabilityJitsi):
        return callInfo->hasCapabilityJitsi;

    case static_cast<int>(Roles::HasAvatar):
        return callInfo->contactInfo.contact ? callInfo->contactInfo.contact->hasAvatar() : false;

    case static_cast<int>(Roles::AvatarPath):
        return callInfo->contactInfo.contact && callInfo->contactInfo.contact->hasAvatar()
                ? callInfo->contactInfo.contact->avatarPath()
                : "";

    case static_cast<int>(Roles::HasIncomingAudioLevel):
        return callInfo->incomingAudioLevel > 0.3;

    case static_cast<int>(Roles::HasMetadata):
        return callInfo->hasMetadata;

    case static_cast<int>(Roles::QualityLevel):
        return QVariant::fromValue(callInfo->qualityLevel);

    case static_cast<int>(Roles::SecurityLevel):
        return QVariant::fromValue(callInfo->securityLevel);

    case static_cast<int>(Roles::RemoteUri):
    default:
        return callInfo->remoteUri;
    }
}
