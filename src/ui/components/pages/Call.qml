pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import base

Item {
    id: control
    focus: true

    LoggingCategory {
        id: category
        name: "gonnect.qml.Call"
        defaultLogLevel: LoggingCategory.Warning
    }

    readonly property alias selectedCallItem: callSideBar.selectedCallItem

    // INFO: RTT is currently limited to 1:1 calls, see GONGONNECT-396
    property bool isRttEnabled: !SIPCallManager.isConferenceMode
                                && RTTProvider.isEstablishedCall
                                && RTTProvider.isRttCall
                                && (RTTProvider.hasMessages || RTTProvider.showRealTimeTextConsole)

    // The avatar should grow in relation to card height, but only to maximum of 202-254 px
    property int maxAvatarSize: control.isRttEnabled ? 202 : 254

    Keys.onPressed: (keyEvent) => {

                        if (keyEvent.key === Qt.Key_V && (keyEvent.modifiers & Qt.ControlModifier)) {
                            if (ClipboardHelper.hasImage()) {
                                keyEvent.accepted = false
                                control.useImageFromClipboard()
                                return
                            }
                        }

                        const callItem = callSideBar.selectedCallItem

                        if (keyEvent.isAutoRepeat || !callItem) {
                            return
                        }

                        const key = keyEvent.text.toUpperCase()
                        const dtmfKeys = [ "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "#", "*", "A", "B", "C", "D" ]

                        if (dtmfKeys.includes(key)) {
                            keyEvent.accepted = true
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
                verticalDragbarDummy.x: (control.LayoutMirroring.enabled ? 1/4 : 3/4) * control.width
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

    function useImageFromClipboard() {
        callSideBar.useImageFromClipboard()
    }

    Card {
        id: callMainCard
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
            right: verticalDragbarDummy.left

            leftMargin: Theme.d * 2
            bottomMargin: 8
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
                } else if (topBar.callItem) {
                    SIPCallManager.endCall(topBar.callItem.accountId, topBar.callItem.callId)
                } else {
                    console.error(category, "cannot hang up because missing call item")
                }
            }

            onAcceptCallClicked: () => {
                SIPCallManager.acceptCall(topBar.callItem.accountId, topBar.callItem.callId)
            }
        }

        Column {
            anchors {
                top: topBar.bottom
                left: parent.left
                right: parent.right
                bottom: nameLabel.top

                topMargin: (control.isRttEnabled || (callRoutingGrid.visible && callRoutingRep.count))
                           ? 30
                           : Math.max(Theme.d * 2, Theme.d * 2 + callMainCard.height / 2 - 254)
                bottomMargin: 15
            }

            Loader {
                id: avatarLoader
                width: parent.width
                height: control.isRttEnabled
                        ? parent.height / 2
                        : (callRoutingGrid.visible && callRoutingRep.count)
                          ? avatarLoader.implicitHeight
                          : parent.height
                sourceComponent: SIPCallManager.isConferenceMode ? multiAvatarComponent : singleAvatarComponent
            }

            Component {
                id: singleAvatarComponent

                CallerBigAvatar {
                    bubbleSize: Math.min(control.maxAvatarSize / 850 * callMainCard.height, control.maxAvatarSize)
                    name: callSideBar.selectedCallItem?.contactName ?? ""
                    avatarUrl: callSideBar.selectedCallItem?.hasAvatar ? ("file://" + callSideBar.selectedCallItem.avatarPath) : ""
                    isIncoming: topBar.isIncoming
                    isEstablished: topBar.isEstablished
                    isInProgress: topBar.isInProgress
                    isIncomingAudioLevel: callSideBar.selectedCallItem?.hasIncomingAudioLevel ?? false
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }

            Component {
                id: multiAvatarComponent

                // Additional item is necessary to center the row as the Loader uses the maximum available space
                Item {
                    id: multiAvatarContainer

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
                                required property bool isInProgress
                                required property bool isIncoming

                                CallerBigAvatar {
                                    id: bigAvatar
                                    bubbleSize: Math.min(control.maxAvatarSize / 850 * callMainCard.height, control.maxAvatarSize)
                                    name: callerDelg.contactName
                                    avatarUrl: callerDelg.avatarPath
                                    isIncoming: callerDelg.isIncoming
                                    isEstablished: callerDelg.isEstablished
                                    isInProgress: callerDelg.isInProgress
                                    isIncomingAudioLevel: callerDelg.hasIncomingAudioLevel
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                            }
                        }
                    }
                }
            }

            Loader {
                id: rttLoader
                width: parent.width / 2
                height: control.isRttEnabled ? parent.height / 2 : 0
                anchors.horizontalCenter: parent.horizontalCenter
                sourceComponent: control.isRttEnabled ? rttComponent : undefined
            }

            Component {
                id: rttComponent

                RTTDisplay {
                    id: rttDisplay
                }
            }

            Column {
                id: callRoutingGrid
                visible: !control.isRttEnabled && callRoutingRep.count > 0
                spacing: 5
                anchors.horizontalCenter: parent.horizontalCenter
                topPadding: 30

                Repeater {
                    id: callRoutingRep
                    model: {
                        const callItem = callSideBar.selectedCallItem
                        if (callItem && (callItem.isIncoming || callItem.isEstablished)) {
                            return CallRoutingHelper.routingHopsForCall(callItem.accountId, callItem.callId)
                        }
                        return null
                    }

                    delegate: Item {
                        id: hop
                        anchors.horizontalCenter: parent?.horizontalCenter
                        implicitWidth: mainLabel.implicitWidth
                        implicitHeight: mainLabel.y + mainLabel.implicitHeight

                        required property int index
                        required property string phoneNumber
                        required property string reasonText
                        required property string contactName

                        readonly property string contactString: hop.contactName ? `${hop.contactName} (${hop.phoneNumber})` : hop.phoneNumber

                        Label {
                            id: arrowLabel
                            visible: hop.index > 0
                            text: '↓'
                            color: Theme.secondaryTextColor
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        Label {
                            id: mainLabel
                            text: hop.reasonText ? `${hop.contactString} (${hop.reasonText})` : hop.contactString
                            color: Theme.secondaryTextColor
                            anchors {
                                top: arrowLabel.visible ? arrowLabel.bottom : parent.top
                                topMargin: arrowLabel.visible ? 5 : 0
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

            Accessible.role: Accessible.StaticText
            Accessible.name: nameLabel.text
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

                Accessible.ignored: true
            }

            Label {
                id: dtmfFeedbackLabel
                anchors.centerIn: parent
                font.pixelSize: 50

                Accessible.ignored: true
            }

            Accessible.ignored: true
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

        Accessible.role: Accessible.Border
        Accessible.name: qsTr("Drag bar")

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
                minimum: control.LayoutMirroring.enabled ? 300 : (1/2 * control.width)
                maximum: control.LayoutMirroring.enabled ? (1/2 * control.width) : (control.width - 300)
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
            roomsAggregator: AggregatedDirectRoomsOfContact {
                contact: control.selectedCallItem?.contact ?? null
            }
        }
    }
}
