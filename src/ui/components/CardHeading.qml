pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    implicitWidth: lbl.implicitWidth + lbl.anchors.leftMargin + lbl.anchors.rightMargin
    height: 46

    property alias text: lbl.text

    Label {
        id: lbl
        font.pixelSize: 16
        font.weight: Font.Medium
        elide: Text.ElideRight
        color: Theme.secondaryTextColor
        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left
            right: parent.right
            leftMargin: 20
            rightMargin: 20
        }
    }

    Rectangle {
        color: Theme.borderColor
        height: 1
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
    }
}
