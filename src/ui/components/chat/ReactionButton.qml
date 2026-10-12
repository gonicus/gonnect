pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import base

Item {
    id: control

    implicitHeight: 24
    implicitWidth: (control.hasIcon ? iconLabel.implicitWidth : emojiLabel.implicitWidth)
                   + textLabel.implicitWidth
                   + control.leftPadding
                   + control.rightPadding
                   + ((control.hasIcon || emojiLabel.text !== "") && textLabel.text !== "" ? control.spacing : 0)

    property alias emoji: emojiLabel.text
    property alias iconPath: iconLabel.icon.source
    property alias text: textLabel.text
    property bool highlighted

    property int leftPadding: 6
    property int rightPadding: 6
    property int spacing: 4

    signal clicked

    readonly property bool hasIcon: control.iconPath.toString() !== ""

    Rectangle {
        id: background
        radius: 6
        anchors.fill: parent
        color: control.highlighted
               ? Theme.backgroundOffsetColor
               : (hoverHandler.hovered
                  ? Theme.backgroundOffsetHoveredColor
                  : Theme.backgroundSecondaryColor)
        border {
            width: 1
            color: control.highlighted
                   ? Theme.highlightColor
                   : (hoverHandler.hovered
                      ? Theme.borderHeaderIconHovered
                      : Theme.borderColor)
        }
    }

    Label {
        id: emojiLabel
        visible: !control.hasIcon
        font {
            family: "Noto Color Emoji"
            pixelSize: 14
        }
        anchors {
            left: parent.left
            leftMargin: control.leftPadding
            verticalCenter: parent.verticalCenter
            verticalCenterOffset: 1
        }
    }

    IconLabel {
        id: iconLabel
        visible: control.hasIcon
        icon {
            width: 16
            height: 16
            color: textLabel.color
        }
        anchors {
            left: parent.left
            leftMargin: control.leftPadding
            verticalCenter: parent.verticalCenter
        }
    }

    Label {
        id: textLabel
        anchors {
            left: control.hasIcon
                  ? iconLabel.right
                  : (emojiLabel.text !== "" ? emojiLabel.right : parent.left)
            leftMargin: (control.hasIcon || emojiLabel.text !== "") ? control.spacing : control.leftPadding
            verticalCenter: parent.verticalCenter
        }
    }

    HoverHandler {
        id: hoverHandler
        cursorShape: Qt.PointingHandCursor
    }

    TapHandler {
        onTapped: () => control.clicked()
    }
}
