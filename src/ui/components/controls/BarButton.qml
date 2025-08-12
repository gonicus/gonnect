pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base

Item {
    id: control
    width: control.implicitWidth
    height: control.implicitHeight
    implicitHeight: 64
    implicitWidth: 60 + (dropDownIconContainer.visible ? 17 : 0)

    signal clicked
    signal dropDownClicked

    property alias text: buttonLabel.text
    property alias iconText: buttonIcon.text
    property alias iconPath: buttonIcon.icon.source
    property alias iconColor: buttonIcon.icon.color
    property bool highlighted: false
    property alias showDropdownButton: dropDownIconContainer.visible
    property alias showIndicatorBadge: indicatorBadge.visible

    states: [
        State {
            when: control.highlighted
            PropertyChanges {
                buttonLabel.color: Theme.primaryTextColor
                buttonLabel.font.weight: Font.Medium
            }
        },
        State {  // Hovered
            when: control.enabled && buttonHoverHandler.hovered
            PropertyChanges {
                buttonLabel.color: Theme.foregroundHeaderIcons
            }
        }
    ]

    Item {
        id: mainTapContainer
        width: 60
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }

        Label {
            id: buttonLabel
            color: Theme.inactiveTextColor
            font.pixelSize: 11
            horizontalAlignment: Label.AlignHCenter
            anchors {
                bottom: parent.bottom
                bottomMargin: 10

                left: parent.left
                right: parent.right
                leftMargin: 10
                rightMargin: 10
            }
        }

        IconLabel {
            id: buttonIcon
            color: buttonLabel.color
            icon {
                width: 20
                height: 20
                color: buttonLabel.color
            }
            anchors {
                centerIn: parent
                verticalCenterOffset: -10
            }
        }

        Rectangle {
            id: indicatorBadge
            x: buttonIcon.x + 14
            y: buttonIcon.y + 1
            visible: false
            width: 6
            height: indicatorBadge.width
            radius: indicatorBadge.width / 2
            color: Theme.redColor
        }

        HoverHandler {
            id: buttonHoverHandler
        }

        TapHandler {
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onTapped: (_, button) => {
                if (button === Qt.LeftButton) {
                    control.clicked()
                } else if (button === Qt.RightButton) {
                    control.dropDownClicked()
                }
            }
        }
    }

    Item {
        id: dropDownIconContainer
        visible: false
        width: 17
        anchors {
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }

        IconLabel {
            id: dropDownIcon
            color: control.enabled && dropDownButtonHoverHandler.hovered ? Theme.foregroundHeaderIcons : Theme.inactiveTextColor
            anchors {
                centerIn: parent
                horizontalCenterOffset: -5
            }
            icon {
                source: Icons.goDown
                color: control.enabled && dropDownButtonHoverHandler.hovered ? Theme.foregroundHeaderIcons : Theme.inactiveTextColor
                width: 12
                height: 12
            }
        }

        HoverHandler {
            id: dropDownButtonHoverHandler
        }

        TapHandler {
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onTapped: () => control.dropDownClicked()
        }
    }
}
