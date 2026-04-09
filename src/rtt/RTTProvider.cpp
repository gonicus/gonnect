#include "RTTProvider.h"

RTTProvider::RTTProvider(QObject *parent) : QObject(parent)
{
    m_model = new RTTModel(this);

    m_manager = &SIPCallManager::instance();
    connect(m_manager, &SIPCallManager::establishedCallsCountChanged, this, [this]() {
        m_call = m_manager->getCurrentCall();

        if (m_call) {
            // TODO: Disconnect on update!
            connect(m_call, &SIPCall::rttBubbleChanged, this, [this](QString message) {
                if (m_newMessage) { // TODO: Safe, cause of ST-event loop?
                    m_model->addMessage(QDateTime::currentMSecsSinceEpoch(), "", message, false);
                    m_newMessage = false;
                } else {
                    m_model->updateMessage(message, false);
                }
            });
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

    ListView {
        anchors.fill: parent
        model: RTTProvider.model

        delegate: ItemDelegate {
            width: parent.width
            text: model.message

            contentItem: Text {
                text: model.message
                color: model.isMe ? "blue" : "gray"
                font.italic: model.isFinished
            }
        }
    }

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

    TextField {
        id: rttInput
        placeholderText: "..."
        onAccepted: focus = false // needed?

        Keys.onPressed: (event) => {
            if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
                RTTProvider.call.rttSendLineSeperator();
                event.accepted = true
            } else if (event.key === Qt.Key_Backspace) {
                RTTProvider.call.rttSendBackspace()
            } else if (event.key === Qt.Key_G && (event.modifiers & Qt.ControlModifier)) {
                // Which key should trigger this?
                RTTProvider.call.rttSendBell()
                event.accepted = true
            }
        }

        onTextEdited: {
            let lastChar = text.charAt(cursorPosition - 1);
            if (lastChar !== "") {
                RTTProvider.call.rttSend(lastChar);
            }
        }
    }
*/
