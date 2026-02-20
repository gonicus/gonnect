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

    Accessible.role: Accessible.ColumnHeader
    Accessible.name: qsTr("Search result category: ") + control.headerText
    Accessible.description: qsTr("Divider for the individual search result items by category")
    Accessible.focusable: true

    Label {
        id: headerLabel
        font.weight: Font.Medium
        anchors {
            left: parent.left
            right: parent.right
        }

        Accessible.ignored: true
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
