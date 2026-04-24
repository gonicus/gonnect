#include "GlobalCallState.h"
#include "SIPCallManager.h"

#include "RTTProvider.h"

RTTProvider::RTTProvider(QObject *parent) : QObject(parent)
{
    m_model = new RTTModel(this);

    connect(m_model, &RTTModel::rowsInserted, this, &RTTProvider::hasMessagesChanged);
    connect(m_model, &RTTModel::rowsRemoved, this, &RTTProvider::hasMessagesChanged);
    connect(m_model, &RTTModel::modelReset, this, &RTTProvider::hasMessagesChanged);

    // TODO: Only active if showRealTimeTextConsole == true
    // or RTT message received (RTTProvider::hasMessages)
    // And limit to one-to-one calls?

    connect(&GlobalCallState::instance(), &GlobalCallState::callInForegroundChanged, this,
            [this]() {
                // Disconnect from RTT signals of the old call
                if (m_changed) {
                    disconnect(m_changed);
                    m_changed = QMetaObject::Connection();
                }
                if (m_committed) {
                    disconnect(m_committed);
                    m_committed = QMetaObject::Connection();
                }
                m_call = nullptr;

                // Clear model containing the RTT messages
                m_model->reset();
                m_newMessage = true;

                // Connect to RTT signals of the new call
                m_state = GlobalCallState::instance().callInForeground();
                if (m_state) {
                    m_call = SIPCallManager::instance().findCallById(m_state->uuid());
                }

                if (m_call) {
                    m_changed = connect(
                            m_call, &SIPCall::rttBubbleChanged, this, [this](QString message) {
                                if (m_newMessage) {
                                    m_model->addMessage(QDateTime::currentMSecsSinceEpoch(),
                                                        message, false);
                                    m_newMessage = false;
                                } else {
                                    m_model->updateMessage(message, false, false);
                                }
                            });
                    m_committed = connect(m_call, &SIPCall::rttBubbleCommitted, this,
                                          [this](QString message) {
                                              m_model->updateMessage(message, false, true);
                                              m_newMessage = true;
                                          });
                }
            });
}

RTTProvider::~RTTProvider()
{
    if (m_changed) {
        disconnect(m_changed);
        m_changed = QMetaObject::Connection();
    }
    if (m_committed) {
        disconnect(m_committed);
        m_committed = QMetaObject::Connection();
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
