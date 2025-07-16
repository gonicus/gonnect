pragma ComponentBehavior: Bound

import QtQuick
import base

Item {
    id: control
    implicitWidth: col.implicitWidth
    implicitHeight: col.y + col.implicitHeight + 20

    property alias title: label.text
    property alias spacing: col.spacing

    default property alias content: col.children

    Card {
        id: background
        anchors {
            top: label.bottom
            topMargin: 10
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
    }

    Label {
        id: label
        font.weight: Font.DemiBold
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
    }

    Column {
        id: col
        anchors {
            top: label.bottom
            topMargin: 30
            left: parent.left
            right: parent.right
            leftMargin: 20
            rightMargin: 20
        }
    }
}
