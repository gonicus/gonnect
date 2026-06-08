pragma ComponentBehavior: Bound

import QtQuick
import base

Item {
    id: control
    implicitHeight: 40
    implicitWidth: sectionHeaderLabel.implicitWidth

    property alias text: sectionHeaderLabel.text

    Label {
        id: sectionHeaderLabel
        font.weight: Font.Medium
        color: Theme.secondaryTextColor
        elide: Label.ElideRight
        verticalAlignment: Label.AlignVCenter
        anchors {
            top: parent.top
            bottom: sectionHeaderSeparator.top
            left: parent.left
            right: parent.right
        }
    }

    Rectangle {
        id: sectionHeaderSeparator
        height: 1
        color: Theme.borderColor
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            bottomMargin: 10
        }
    }
}
