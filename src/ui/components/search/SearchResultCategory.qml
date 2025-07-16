pragma ComponentBehavior: Bound

import QtQuick
import base

Item {
    id: control
    implicitWidth: headerLabel.implicitWidth
    implicitHeight: contentContainer.y + contentContainer.implicitHeight

    default property alias content: contentContainer.children

    readonly property alias searchChildren: contentContainer.children

    property alias headerText: headerLabel.text

    Label {
        id: headerLabel
        font.weight: Font.Medium
        anchors {
            left: parent.left
            right: parent.right
        }
    }

    Rectangle {
        id: headerLine
        height: 1
        color: Theme.borderColor
        anchors {
            left: parent.left
            right: parent.right
            top: headerLabel.bottom
        }
    }

    Flow {
        id: contentContainer

        anchors {
            left: parent.left
            right: parent.right
            top: headerLine.bottom
        }
    }
}
