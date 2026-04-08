#include "RTTProvider.h"

RTTProvider::RTTProvider(QObject *parent) : QObject(parent)
{
    m_model = new RTTModel(this);
    m_callManager = &SIPCallManager::instance(); // TODO: Concept, remove later
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
*/
