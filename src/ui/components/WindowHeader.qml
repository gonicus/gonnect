pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Rectangle {
    id: control
    color: control.active ? Theme.backgroundHeader : Theme.backgroundHeaderInactive
    height: 44
    radius: 8

    readonly property Window window: control.Window.window
    readonly property bool active: control.Window.window?.active ?? false

    // This rectangle makes the bottom rounded corners of rect straight
    Rectangle {
        height: control.radius
        color: control.color
        anchors {
            left: control.left
            right: control.right
            bottom: control.bottom
        }
    }

    DragHandler {
        id: systemDragHandler
        target: null
        onActiveChanged: () => {
            if (systemDragHandler.active) {
                control.window.startSystemMove()
            }
        }
    }

    Label {
        id: headerTitle
        anchors.centerIn: parent
        color: control.active ? Theme.foregroundHeaderIcons : Theme.foregroundHeaderIconsInactive
        text: control.window?.title ?? ""
    }

    Row {
        spacing: 3
        anchors {
            right: parent.right
            rightMargin: 8
            verticalCenter: parent.verticalCenter
        }

        // HeaderIconButton {
        //     iconSource: Icons.goDown
        //     active: control.active
        //     anchors.verticalCenter: parent.verticalCenter
        //     onClicked: () => console.log('TODO: Minimize')
        // }

        HeaderIconButton {
            iconSource: Icons.mobileCloseApp
            active: control.active
            iconSize: 10
            anchors.verticalCenter: parent.verticalCenter
            onClicked: () => control.window?.close()
        }
    }
}
