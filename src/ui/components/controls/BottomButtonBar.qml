pragma ComponentBehavior: Bound

import QtQuick
import base

Rectangle {
    id: buttonBar
    height: 36
    color: Theme.backgroundOffsetColor
    bottomLeftRadius: 12
    bottomRightRadius: 12

    property alias rightContent: rightButtonRow.data
    property alias centerContent: centerContainer.data

    default property alias leftContent: leftButtonRow.data

    Row {
        id: leftButtonRow
        leftPadding: 5
        anchors {
            top: parent.top
            left: parent.left
            bottom: parent.bottom
        }
    }

    Item {
        id: centerContainer
        anchors {
            top: parent.top
            bottom: parent.bottom
            left: leftButtonRow.right
            right: rightButtonRow.left
        }
    }

    Row {
        id: rightButtonRow
        rightPadding: leftButtonRow.leftPadding
        anchors {
            top: parent.top
            right: parent.right
            bottom: parent.bottom
        }
    }
}
