#include <QLoggingCategory>
#include "GOnnectDBusAPIAdaptor.h"
#include "GOnnectDBusAPI.h"
#include "SIPCallManager.h"
#include "PhoneNumberUtil.h"

GOnnectDBusAPI::GOnnectDBusAPI(QObject *parent) : QObject(parent)
{
    m_apiAdaptor = new GOnnectDBusAPIAdaptor(this);

    auto con = QDBusConnection::sessionBus();
    if (con.isConnected()) {
        con.registerObject(QString(FLATPAK_APP_PATH) + "/api", this);
        con.registerService(FLATPAK_APP_ID);
    }
}

GOnnectDBusAPI::~GOnnectDBusAPI() { }

void GOnnectDBusAPI::registerCallState(ICallState *state)
{
    if (state) {
        const QString uuid = state->uuid();

        connect(state, &ICallState::callStateChanged, this, [this, state, uuid]() {
            const auto cs = state->callState();
            const auto changeMask = m_oldCallState ^ cs;

            m_oldCallState = cs;

            if (changeMask & ICallState::State::RingingIncoming
                && cs & ICallState::State::RingingIncoming) {

                if (auto call = SIPCallManager::instance().findCallById(uuid)) {
                    const auto contactInfo =
                            PhoneNumberUtil::instance().contactInfoBySipUrl(call->sipUrl());

                    emit m_apiAdaptor->Ringing(uuid, contactInfo.toString());
                }
            }

            if (changeMask & ICallState::State::CallActive && cs & ICallState::State::CallActive) {
                emit m_apiAdaptor->CallAnswered(uuid);
            }
        });

        connect(state, &ICallState::destroyed, this, [this, uuid]() {
            m_states.remove(uuid);
            emit m_apiAdaptor->CallEnded(uuid);
        });
    }
}

void GOnnectDBusAPI::AddMetaData(const QString &Id, const QString &Data)
{
    SIPCallManager::instance().addMetadata(Id, Data);
}

QString GOnnectDBusAPI::Call(const QString &uri)
{
    return SIPCallManager::instance().call(uri);
}

QVariantList GOnnectDBusAPI::CallIds()
{
    return QVariant(SIPCallManager::instance().callIds()).toList();
}

void GOnnectDBusAPI::HangupAllCalls()
{
    SIPCallManager::instance().endAllCalls();
}

void GOnnectDBusAPI::HangupCall(const QString &id)
{
    SIPCallManager::instance().endCall(id);
}
