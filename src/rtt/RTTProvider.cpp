#include "SelectionState.h"
#include "SIPCallManager.h"

#include "RTTProvider.h"

RTTProvider::RTTProvider(QObject *parent) : QObject(parent)
{
    m_showRealTimeTextConsole =
            m_settings.value("account0/showRealTimeTextConsole", false).toBool();

    m_model = new RTTModel(this);

    connect(m_model, &RTTModel::rowsInserted, this, &RTTProvider::hasMessagesChanged);
    connect(m_model, &RTTModel::rowsRemoved, this, &RTTProvider::hasMessagesChanged);
    connect(m_model, &RTTModel::modelReset, this, &RTTProvider::hasMessagesChanged);

    connect(&SelectionState::instance(), &SelectionState::callInForegroundChanged, this, [this]() {
        // Disconnect from signals of the old call
        if (m_establishedCall) {
            disconnect(m_establishedCall);
            m_establishedCall = QMetaObject::Connection();
        }
        if (m_rttCall) {
            disconnect(m_rttCall);
            m_rttCall = QMetaObject::Connection();
        }
        if (m_rttBubbleChanged) {
            disconnect(m_rttBubbleChanged);
            m_rttBubbleChanged = QMetaObject::Connection();
        }
        if (m_rttBubbleCommitted) {
            disconnect(m_rttBubbleCommitted);
            m_rttBubbleCommitted = QMetaObject::Connection();
        }
        m_call = nullptr;

        // Clear model containing the RTT messages
        m_model->reset();
        m_newMessage = true;

        // Connect to signals of the new call
        m_state = SelectionState::instance().callInForeground();
        if (m_state) {
            m_call = SIPCallManager::instance().findCallById(m_state->uuid());
        }

        if (m_call) {
            m_establishedCall =
                    connect(m_call, &SIPCall::establishedChanged, this,
                            &RTTProvider::isEstablishedCallChanged, Qt::QueuedConnection);

            m_rttCall = connect(m_call, &SIPCall::hasRttChanged, this,
                                &RTTProvider::isRttCallChanged, Qt::QueuedConnection);

            m_rttBubbleChanged = connect(
                    m_call, &SIPCall::rttBubbleChanged, this,
                    [this](QString message) {
                        if (m_newMessage) {
                            m_model->addMessage(QDateTime::currentMSecsSinceEpoch(), message,
                                                false);
                            m_newMessage = false;
                        } else {
                            m_model->updateMessage(message, false, false);
                        }
                    },
                    Qt::QueuedConnection);

            m_rttBubbleCommitted = connect(
                    m_call, &SIPCall::rttBubbleCommitted, this,
                    [this](QString message) {
                        m_model->updateMessage(message, false, true);
                        m_newMessage = true;
                    },
                    Qt::QueuedConnection);
        }
    });
}

RTTProvider::~RTTProvider()
{
    if (m_establishedCall) {
        disconnect(m_establishedCall);
        m_establishedCall = QMetaObject::Connection();
    }
    if (m_rttCall) {
        disconnect(m_rttCall);
        m_rttCall = QMetaObject::Connection();
    }
    if (m_rttBubbleChanged) {
        disconnect(m_rttBubbleChanged);
        m_rttBubbleChanged = QMetaObject::Connection();
    }
    if (m_rttBubbleCommitted) {
        disconnect(m_rttBubbleCommitted);
        m_rttBubbleCommitted = QMetaObject::Connection();
    }
    m_call = nullptr;

    if (m_model) {
        delete m_model;
        m_model = nullptr;
    }
}

bool RTTProvider::hasMessages()
{
    if (m_model) {
        return m_model->rowCount() > 0;
    }

    return false;
}

bool RTTProvider::isEstablishedCall()
{
    if (m_call) {
        return m_call->isEstablished();
    }

    return false;
}

bool RTTProvider::isRttCall()
{
    if (m_call) {
        return m_call->hasRtt();
    }

    return false;
}

void RTTProvider::rttSend(const QString &text)
{
    if (m_call) {
        m_call->rttSend(text);
    }
}

void RTTProvider::rttSendLineSeperator()
{
    if (m_call) {
        m_call->rttSendLineSeperator();
    }
}

void RTTProvider::rttSendCRLF()
{
    if (m_call) {
        m_call->rttSendCRLF();
    }
}

void RTTProvider::rttSendBackspace()
{
    if (m_call) {
        m_call->rttSendBackspace();
    }
}

void RTTProvider::rttSendBell()
{
    if (m_call) {
        m_call->rttSendBell();
    }
}
