pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base

ListView {
    id: togglerList
    implicitHeight: 30
    leftMargin: 20
    rightMargin: 20
    spacing: 20
    orientation: ListView.Horizontal
    model: TogglerProxyModel {
        displayFilter: Toggler.STATUS
        TogglerModel {}
    }
    delegate: Switch {
        id: delg
        padding: 0
        text: delg.name
        checked: delg.isActive
        enabled: !delg.isBusy
        anchors.verticalCenter: parent?.verticalCenter
        indicator: delg.isBusy ? busyIndicatorItem : idleIndicatorItem

        required property string id
        required property string name
        required property bool isActive
        required property bool isBusy

        onToggled: () => TogglerManager.toggleToggler(delg.id)

        Rectangle {
            id: idleIndicatorItem
            implicitWidth: 36
            implicitHeight: 20
            x: delg.leftPadding
            y: parent?.height / 2 - height / 2
            radius: 13
            visible: !delg.isBusy
            color: delg.checked ? Theme.accentColor : Theme.backgroundColor
            border.color: delg.checked ? Theme.accentColor : Theme.borderColor

            Behavior on color { ColorAnimation { duration: 100 } }
            Behavior on border.color { ColorAnimation { duration: 100 } }

            Rectangle {
                x: delg.checked ? (parent.width - width - 2) : 2
                y: parent.height / 2 - height / 2
                width: 16
                height: width
                radius: width / 2
                color: delg.checked ? Theme.backgroundColor : Theme.secondaryTextColor

                Behavior on x { NumberAnimation { duration: 100 } }
                Behavior on color { ColorAnimation { duration: 100 } }
            }
        }

        Item {
            id: busyIndicatorItem
            implicitWidth: 36
            implicitHeight: 20
            visible: delg.isBusy
            x: delg.leftPadding
            y: parent ? (parent.height / 2 - height / 2) : 0

            IconLabel {
                anchors.centerIn: parent
                icon {
                    width: 16
                    height: 16
                    color: Theme.secondaryTextColor
                    source: Icons.viewRefresh
                }
            }
        }
    }
}
