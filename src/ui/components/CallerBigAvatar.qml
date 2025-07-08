pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    implicitWidth: volumeMeterBg.width
    implicitHeight: callingLabel.y + callingLabel.implicitHeight

    property alias bubbleSize: avatarImage.size
    property alias name: otherName.text
    property alias avatarUrl: avatarImage.source

    property bool isIncoming
    property bool isEstablished
    property bool isIncomingAudioLevel

    Rectangle {
        id: volumeMeterBg
        anchors.centerIn: avatarImage
        width: 1.15 * avatarImage.size
        height: volumeMeterBg.width
        radius: volumeMeterBg.width / 2
        color: Theme.backgroundOffsetHoveredColor
        opacity: control.isIncomingAudioLevel ? 1.0  : 0.0

        Behavior on opacity {
            NumberAnimation { duration: 300 }
        }
    }

    AvatarImage {
        id: avatarImage
        initials: ViewHelper.initials(control.name)
        anchors {
            horizontalCenter: parent.horizontalCenter
            top: parent.top
        }
    }

    Label {
        id: otherName
        font.pixelSize: 26
        anchors {
            horizontalCenter: parent.horizontalCenter
            top: avatarImage.bottom
            topMargin: 50
        }
    }

    Label {
        font.pixelSize: 22
        text: qsTr("is calling...")
        color: Theme.secondaryTextColor
        visible: control.isIncoming && !control.isEstablished
        anchors {
            horizontalCenter: parent.horizontalCenter
            top: otherName.bottom
            topMargin: 30
        }
    }

    Label {
        id: callingLabel
        font.pixelSize: 22
        text: qsTr("Calling...")
        color: Theme.secondaryTextColor
        visible: !control.isIncoming && !control.isEstablished
        anchors {
            horizontalCenter: parent.horizontalCenter
            top: otherName.bottom
            topMargin: 30
        }
    }
}
