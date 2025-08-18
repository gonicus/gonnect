import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base

Item {
    id: control
    implicitHeight: control.iconSize
    implicitWidth: firstAidLabel.x + firstAidLabel.implicitWidth
    visible: GlobalInfo.hasEmergencyNumbers

    property int iconSize: 24

    Rectangle {
        id: firstAidIcon
        width: control.iconSize
        height: control.iconSize
        radius: control.iconSize / 2
        color: Qt.rgba(1, 1, 1)

        Rectangle {
            id: verticalBar
            color: Qt.rgba(1, 0, 0)
            width: 1/4 * control.iconSize
            anchors {
                horizontalCenter: parent.horizontalCenter
                top: parent.top
                bottom: parent.bottom
                topMargin: 1/12 * control.iconSize
                bottomMargin: 1/12 * control.iconSize
            }
        }

        Rectangle {
            id: horizontalBar
            color: Qt.rgba(1, 0, 0)
            height: 1/4 * control.iconSize
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
                right: parent.right
                leftMargin: 1/12 * control.iconSize
                rightMargin: 1/12 * control.iconSize
            }
        }
    }

    Label {
        id: firstAidLabel
        text: qsTr("First Aid")
        color: firstAidHoverHandler.hovered ? Theme.primaryTextColor : Theme.inactiveTextColor
        anchors {
            verticalCenter: control.verticalCenter
            left: firstAidIcon.right
            leftMargin: 10
        }
    }

    HoverHandler {
        id: firstAidHoverHandler
    }
    TapHandler {
        onTapped: () => ViewHelper.showFirstAid()
    }
}
