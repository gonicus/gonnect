pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    focus: true

    readonly property alias selectedCallItem: callSideBar.selectedCallItem

    Keys.onPressed: (event) => {
                        const callItem = callSideBar.selectedCallItem

                        if (event.isAutoRepeat || !callItem) {
                            return
                        }

                        const key = event.text.toUpperCase()
                        const dtmfKeys = [ "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "#", "*", "+", "A", "B", "C", "D", "," ]

                        if (dtmfKeys.includes(key)) {
                            event.accepted = true
                            SIPCallManager.sendDtmf(callItem.accountId, callItem.callId, key)

                            dtmfFeedbackLabel.text = key
                            dtmfFeedbackRect.dtmfAnimation.restart()
                        }
                    }

    Component.onCompleted: () => control.forceActiveFocus()
    onVisibleChanged: () => {
        if (control.visible) {
            control.forceActiveFocus()
        }
    }

    states: [
        State {
            when: callSideBar.extended
            PropertyChanges {
                verticalDragbarDummyDragHandler.enabled: true
                verticalDragbarDummyHoverHandler.enabled: true
                verticalDragbarDummy.x: 3/4 * control.width
            }

            AnchorChanges {
                target: verticalDragbarDummy
                anchors.right: undefined
            }

            AnchorChanges {
                target: callListCard
                anchors.left: verticalDragbarDummy.right
            }
        }
    ]

    Card {
        id: callMainCard
        anchors {
            left: parent.left
            top: parent.top
            bottom: firstAidButton.top
            right: verticalDragbarDummy.left

            leftMargin: 24
            bottomMargin: 15
        }

        CallButtonBar {
            id: topBar
            height: topBar.implicitHeight
            callItem: callSideBar.selectedCallItem
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
            }

            onHangupClicked: () => {
                if (SIPCallManager.isConferenceMode) {
                    SIPCallManager.endConference()
                } else {
                    SIPCallManager.endCall(topBar.callItem.accountId, topBar.callItem.callId)
                }
            }

            onAcceptCallClicked: () => {
                SIPCallManager.acceptCall(topBar.callItem.accountId, topBar.callItem.callId)
            }
        }

        Loader {
            id: avatarLoader
            sourceComponent: SIPCallManager.isConferenceMode ? multiAvatarComponent : singleAvatarComponent
            anchors {
                top: topBar.bottom
                left: parent.left
                right: parent.right
                bottom: parent.bottom

                topMargin: Math.max(24, 24 + callMainCard.height / 2 - 254)
            }
        }

        Component {
            id: singleAvatarComponent

            CallerBigAvatar {
                bubbleSize: Math.min(254 / 700 * callMainCard.height, 254)  // Grow in relation to card height, but only to maximum of 254 px
                name: callSideBar.selectedCallItem?.contactName ?? ""
                avatarUrl: callSideBar.selectedCallItem?.hasAvatar ? ("file://" + callSideBar.selectedCallItem.avatarPath) : ""
                isIncoming: topBar.isIncoming
                isEstablished: topBar.isEstablished
                isIncomingAudioLevel: callSideBar.selectedCallItem?.hasIncomingAudioLevel ?? false
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }

        Component {
            id: multiAvatarComponent

            Item {
                id: multiAvatarContainer
                // Additional item is necessary to center the row as the Loader uses the maximum available space

                Row {
                    anchors.horizontalCenter: parent?.horizontalCenter

                    Repeater {
                        id: callsRepeater
                        model: CallsProxyModel {
                            onlyEstablishedCalls: true
                            CallsModel {}
                        }
                        delegate: Item {
                            id: callerDelg
                            implicitWidth: Math.max(0.75 * multiAvatarContainer.width / callsRepeater.count, bigAvatar.implicitWidth)
                            implicitHeight: bigAvatar.implicitHeight

                            required property string contactName
                            required property string avatarPath
                            required property bool hasIncomingAudioLevel
                            required property bool isEstablished
                            required property bool isIncoming

                            CallerBigAvatar {
                                id: bigAvatar
                                bubbleSize: Math.min(254 / 700 * callMainCard.height, 254)  // Grow in relation to card height, but only to maximum of 254 px
                                name: callerDelg.contactName
                                avatarUrl: callerDelg.avatarPath
                                isIncoming: callerDelg.isIncoming
                                isEstablished: callerDelg.isEstablished
                                isIncomingAudioLevel: callerDelg.hasIncomingAudioLevel
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }
                    }
                }
            }
        }

        Label {
            id: nameLabel
            font.weight: Font.Medium
            text: {
                if (SIPCallManager.isConferenceMode) {
                    return qsTr("Conference")
                }

                let s = callSideBar.selectedCallItem?.name || callSideBar.selectedCallItem?.phoneNumber || ""

                if (callSideBar.selectedCallItem?.company) {
                    s += ` - ${callSideBar.selectedCallItem.company}`
                }

                return s
            }
            anchors {
                left: parent.left
                bottom: parent.bottom
                leftMargin: 15
                bottomMargin: 10
            }
        }

        Rectangle {
            id: dtmfFeedbackRect
            anchors.centerIn: parent
            width: 100
            height: 100
            radius: 12
            color: Theme.backgroundColor
            opacity: 0

            readonly property SequentialAnimation dtmfAnimation: SequentialAnimation {
                NumberAnimation {
                    target: dtmfFeedbackRect
                    property: 'opacity'
                    to: 1.0
                    duration: 100
                }
                NumberAnimation {
                    target: dtmfFeedbackRect
                    property: 'opacity'
                    easing.type: Easing.InQuad
                    to: 0.0
                    duration: 1200
                }
            }

            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                color: Theme.backgroundOffsetColor
            }

            Label {
                id: dtmfFeedbackLabel
                anchors.centerIn: parent
                font.pixelSize: 50
            }
        }
    }

    Item {
        id: verticalDragbarDummy
        width: 24
        anchors {
            top: parent.top
            bottom: parent.bottom
            right: callListCard.left
        }

        HoverHandler {
            id: verticalDragbarDummyHoverHandler
            enabled: false
            cursorShape: Qt.SplitHCursor
        }

        DragHandler {
            id: verticalDragbarDummyDragHandler
            enabled: false
            yAxis.enabled: false
            xAxis {
                minimum: 1/2 * control.width
                maximum: control.width - 300
            }
        }
    }

    Card {
        id: callListCard
        width: 70
        anchors {
            top: callMainCard.top
            bottom: callMainCard.bottom
            right: parent.right
            rightMargin: callMainCard.anchors.leftMargin
        }

        CallSideBar {
            id: callSideBar
            anchors.fill: parent
            chatAvailable: false
        }
    }

    TogglerList {
        id: togglerList
        visible: togglerList.count > 0
        clip: true
        anchors {
            left: parent.left
            right: firstAidButton.left
            bottom: parent.bottom

            leftMargin: 10
            rightMargin: 10
            bottomMargin: 10
        }
    }

    FirstAidButton {
        id: firstAidButton
        z: 100000
        anchors {
            verticalCenter: togglerList.verticalCenter
            right: callListCard.right
            rightMargin: 10
        }
    }
}
