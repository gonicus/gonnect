#include "SelectionState.h"
#include "SIPCallManager.h"
#include "ICallState.h"
#include "SIPCall.h"

SelectionState::SelectionState(QObject *parent) : QObject{ parent } { }

void SelectionState::setCallInForeground(ICallState *call)
{
    if (m_callInForeground != call) {

        QObject::disconnect(m_callInForegroundDestroyedConnection);
        m_callInForegroundDestroyedConnection = QMetaObject::Connection();

        m_callInForeground = call;

        if (call) {
            m_callInForegroundDestroyedConnection =
                    connect(call, &QObject::destroyed, this,
                            [this](QObject *) { setCallInForeground(nullptr); });
        }

        Q_EMIT callInForegroundChanged();
    }
}

void SelectionState::setCallInForeground(const QString &accountId, int callId)
{
    setCallInForeground(
            qobject_cast<ICallState *>(SIPCallManager::instance().findCall(accountId, callId)));
}
