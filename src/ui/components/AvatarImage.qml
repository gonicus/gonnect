pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import Qt5Compat.GraphicalEffects
import base

Item {
    id: control
    width: control.size
    height: control.size
    implicitWidth: control.size
    implicitHeight: control.size

    property alias initials: initialsLabel.text
    property alias source: img.source
    property int size: 24

    property alias showBuddyStatus: buddyStatusIndicatorContainer.visible
    property alias buddyStatus: buddyStatusIndicator.status
    property alias isBlocked: buddyStatusIndicator.isBlocked

    states: [
        State {
            when: control.source.toString() !== ""  // toString() is necessary, see QTBUG-63629
            PropertyChanges {
                initialBackground.visible: false
                initialsLabel.visible: false
                opacityMask.visible: true
            }
        }
    ]

    Rectangle {
        id: initialBackground
        anchors.fill: parent
        color: Theme.backgroundInitials
        radius: initialBackground.width / 2
    }

    Label {
        id: initialsLabel
        anchors.centerIn: parent
        font.pixelSize: 0.4 * control.size
        color: Theme.foregroundInitials
    }

    Image {
        id: img
        visible: false
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
        sourceSize.width: control.size
        sourceSize.height: control.size
    }

    Rectangle {
        id: mask
        anchors.fill: parent
        color: 'black'
        visible: false
        radius: mask.width / 2
    }

    OpacityMask {
        id: opacityMask
        visible: false
        anchors.fill: parent
        source: img
        maskSource: mask
    }

    Rectangle {
        id: buddyStatusIndicatorContainer
        color: Theme.backgroundColor
        x: control.width / 2 + Math.sqrt(((control.width / 2) * (control.width / 2)) / 2) - buddyStatusIndicatorContainer.width / 2
        y: buddyStatusIndicatorContainer.x
        width: 10
        height: buddyStatusIndicatorContainer.width
        radius: buddyStatusIndicatorContainer.width / 2
        visible: false

        BuddyStatusIndicator {
            id: buddyStatusIndicator
            width: parent.width - 2
            anchors.centerIn: parent
        }
    }
}
