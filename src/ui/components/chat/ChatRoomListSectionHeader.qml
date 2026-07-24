pragma ComponentBehavior: Bound

import QtQuick
import base

Item {
    id: control
    implicitWidth: sectionHeaderLabel.implicitWidth
    height: control.implicitHeight
    implicitHeight: sectionHeaderLabel.implicitHeight
                    + sectionHeaderLabel.anchors.topMargin
                    + sectionHeaderSeparator.anchors.bottomMargin

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
            topMargin: Theme.d * 2
            bottomMargin: Theme.d
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
            bottomMargin: Theme.d
        }
    }
}
