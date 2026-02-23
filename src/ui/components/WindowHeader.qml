pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Rectangle {
    id: control
    color: control.active ? Theme.backgroundHeader : Theme.backgroundHeaderInactive
    height: 44
    radius: 8
    anchors {
        top: parent?.top
        left: parent?.left
        right: parent?.right
    }

    signal toggleMaximized

    property alias showMinimizeButton: minimizeButton.visible
    property alias showMaximizeButton: maximizeButton.visible

    readonly property var window: control.Window.window
    readonly property bool active: control.Window.window?.active ?? false

    Accessible.role: Accessible.Heading
    Accessible.name: qsTr("GOnnect window header")

    // This rectangle makes the bottom rounded corners of rect straight
    Rectangle {
        height: control.radius
        color: control.color
        anchors {
            left: control.left
            right: control.right
            bottom: control.bottom
        }

        Accessible.ignored: true
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

    TapHandler {
        onDoubleTapped: () => control.toggleMaximized()
    }

    Image {
        width: 24
        height: 24
        source: "qrc:/icons/gonnect.svg"
        sourceSize.width: 24
        sourceSize.height: 24
        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left
            leftMargin: 10
        }
    }

    Label {
        id: headerTitle
        anchors.centerIn: parent
        color: control.active ? Theme.foregroundHeaderIcons : Theme.foregroundHeaderIconsInactive
        text: control.window?.title ?? ""

        Accessible.ignored: true
    }

    Row {
        spacing: 3
        anchors {
            right: parent.right
            rightMargin: 8
            verticalCenter: parent.verticalCenter
        }

        HeaderIconButton {
            id: minimizeButton
            iconSource: Icons.goDown
            accessiblePurpose: qsTr("Minimize")
            active: control.active
            anchors.verticalCenter: parent.verticalCenter
            onClicked: () => control.window?.showMinimized()
        }

        HeaderIconButton {
            id: maximizeButton
            iconSource: Icons.goUp
            accessiblePurpose: qsTr("Maximize")
            active: control.active
            anchors.verticalCenter: parent.verticalCenter
            onClicked: () => control.toggleMaximized()
        }

        HeaderIconButton {
            iconSource: Icons.mobileCloseApp
            accessiblePurpose: qsTr("Close QGonnect")
            active: control.active
            iconSize: 10
            anchors.verticalCenter: parent.verticalCenter
            onClicked: () => control.window?.close()
        }
    }
}
