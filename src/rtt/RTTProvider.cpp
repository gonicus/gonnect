#include "RTTProvider.h"

RTTProvider::RTTProvider(QObject *parent) : QObject(parent)
{
    m_model = new RTTModel(this);

    m_manager = &SIPCallManager::instance();
    connect(m_manager, &SIPCallManager::establishedCallsCountChanged, this, [this]() {
        // Disconnect from RTT signals of the old call
        if (m_call) {
            disconnect(m_changed);
            disconnect(m_committed);
        }

        // Clear model containing the RTT messages
        m_model->reset();

        // Connect to RTT signals of the new call
        m_call = m_manager->getCurrentCall();
        if (m_call) {
            m_changed = connect(m_call, &SIPCall::rttBubbleChanged, this, [this](QString message) {
                if (m_newMessage) { // TODO: Safe, cause of ST-event loop?
                    m_model->addMessage(QDateTime::currentMSecsSinceEpoch(), message, false);
                    m_newMessage = false;
                } else {
                    m_model->updateMessage(message, false);
                }
            });
            m_committed =
                    connect(m_call, &SIPCall::rttBubbleCommitted, this, [this](QString message) {
                        m_model->updateMessage(message, true);
                        m_newMessage = true;
                    });
        }
    });
}

/*
This singleton provider class serves as the easily accessible interface to the model

We'd have to fetch the active call from the call manager and connect to SIPCall
rttBubbleChanged / rttBubbleCommitted and hook them up to RTTModel
updateLastMessage / addMessage

QML text field in Call.qml needs linking to SIPCall rtt bubble?

Should be Q_INVOKABLE and triggerable by QML UI TextInput?
    void rttSend(const QString &text);
    void rttSendLineSeperator();
    void rttSendCRLF();
    void rttSendBackspace();
    void rttSendBell();

    -> QML UI part also needs to do addMessage() / updateMessage()

Provider should listen for these SIPCall signals:
    void rttBubbleChanged(QString &text);
    void rttBubbleCommitted(QString &text);
*/
