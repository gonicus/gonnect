import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base

Rectangle {
    id: control
    implicitHeight: 90
    border.width: dropArea.containsDrag ? 5 : 0
    border.color: Theme.borderColor
    color: hoverHandler.hovered
           ? Theme.backgroundOffsetHoveredColor
           : (control.selected
              ? Theme.backgroundOffsetColor
              : 'transparent')

    required property int callId
    required property string accountId
    required property string phoneNumber
    required property string contactName
    required property string city
    required property string country
    required property string company
    required property bool isIncoming
    required property bool isEstablished
    required property date establishedTime
    required property bool isHolding
    required property bool isFinished
    required property bool hasCapabilityJitsi
    required property int statusCode
    required property bool hasIncomingAudioLevel
    required property bool hasAvatar
    required property string avatarPath

    property bool selected: false
    property bool interactive: true
    property bool showHoldButton: true
    property alias showHangupButton: hangupButton.visible
    property int padding: 12

    signal clicked


    states: [
        State {
            when: control.isHolding
            PropertyChanges {
                levelMeter.visible: false
                holdIcon.visible: true
            }
        }
    ]


    VerticalLevelMeter {
        id: levelMeter
        hasAudioLevel: control.hasIncomingAudioLevel
        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left
            leftMargin: 10
        }
    }

    IconLabel {
        id: holdIcon
        visible: false
        icon {
            source: Icons.mediaPlaybackPause
            width: 12
            height: 12
        }
        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left
            leftMargin: 15
        }
    }

    Column {
        id: labelCol
        spacing: 4
        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left
            leftMargin: 40
            right: hangupButton.visible ? hangupButton.left : parent.right
            rightMargin: control.padding
        }

        Label {
            id: nameLabel
            elide: Label.ElideRight
            text: control.contactName || control.phoneNumber
            font.weight: Font.DemiBold
            wrapMode: Label.WordWrap
            maximumLineCount: (companyLabel.text === '' && cityLabel.text === '')
                              ? 3
                              : ((companyLabel.text === '' || cityLabel.text === '')
                                 ? 2
                                 : 1)
            anchors {
                left: parent.left
                right: parent.right
            }
        }

        Label {
            id: companyLabel
            elide: Label.ElideRight
            visible: control.company !== ""
            text: control.company
            color: Theme.secondaryTextColor
            anchors {
                left: parent.left
                right: parent.right
            }
        }

        Label {
            id: cityLabel
            elide: Label.ElideRight
            visible: control.city !== "" || control.country !== ""
            color: Theme.secondaryTextColor
            text: control.city ? control.city : (control.country ? control.country : "")
            anchors {
                left: parent.left
                right: parent.right
            }
        }
    }

    Rectangle {
        id: hangupButton
        color: Theme.redColor
        radius: 4
        width: 24
        height: hangupButton.width
        anchors {
            verticalCenter: parent.verticalCenter
            right: parent.right
            rightMargin: control.padding
        }

        IconLabel {
            anchors.centerIn: parent
            icon {
                width: 16
                height: 15
                source: Icons.callStop
                color: Theme.foregroundWhiteColor
            }
        }

        TapHandler {
            onTapped: SIPCallManager.endCall(control.accountId, control.callId)
        }
    }

    HoverHandler {
        id: hoverHandler
        enabled: control.interactive
    }

    TapHandler {
        onTapped: () => control.clicked()
        enabled: control.interactive
    }

    DragHandler {
        id: dragHandler
        grabPermissions: PointerHandler.CanTakeOverFromAnything
        enabled: control.interactive

        property point originalPosition

        onActiveChanged: () => {
                             if (dragHandler.active) {
                                 // Save position to snap back on drag cancel
                                 dragHandler.originalPosition = Qt.point(control.x, control.y)
                             } else {
                                 const dropResult = control.Drag.drop()

                                 // Snap back to original position
                                 if (dropResult !== Qt.MoveAction) {
                                     control.x = dragHandler.originalPosition.x
                                     control.y = dragHandler.originalPosition.y
                                 }
                             }
                         }
    }

    Drag.active: dragHandler.active
    Drag.source: control
    Drag.hotSpot: Qt.point(control.width / 2, control.height / 2)

    DropArea {
        id: dropArea
        anchors.fill: parent

        onDropped: (dragEvent) => {
                       dragEvent.accept(Qt.MoveAction)
                       SIPCallManager.transferCall(dragEvent.source.accountId, dragEvent.source.callId,
                                                   control.accountId, control.callId)
                   }
    }
}
