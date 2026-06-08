pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Popup {
    id: control
    implicitWidth: control.maxItemWidth + 20
    implicitHeight: Math.min(400, col.implicitHeight) + 20

    signal accepted(string id)

    property alias filterText: proxyModel.filterText
    property alias chatRoom: usersModel.chatRoom

    property int selectedIndex: -1

    readonly property alias count: userRepeater.count

    readonly property int maxItemWidth: {
        let width = 0
        for (let i = 0; i < userRepeater.count; ++i) {
            const item = userRepeater.itemAt(i)
            width = Math.max(width, item.implicitWidth)
        }
        return width
    }

    function idAt(index : int) : string {
        const item = userRepeater.itemAt(index)
        if (item) {
            return item.id
        }
        console.error(category, `Index ${index} is out of bounds (bounds: [0;${userRepeater.count}[)`)
        return ""
    }

    function decrementIndex() {
        let newIndex = control.selectedIndex - 1
        if (newIndex < -1) {
            newIndex = userRepeater.count - 1
        }
        control.selectedIndex = newIndex
        control.scrollCurrentSelectionIntoView()
    }

    function incrementIndex() {
        let newIndex = control.selectedIndex + 1
        if (newIndex >= userRepeater.count) {
            newIndex = -1
        }
        control.selectedIndex = newIndex
        control.scrollCurrentSelectionIntoView()
    }

    function scrollCurrentSelectionIntoView() {
        const item = userRepeater.itemAt(control.selectedIndex)
        if (item) {
            if (item.y < flickable.contentY) {
                flickable.contentY = item.index * item.height
            } else if (item.y + item.height > flickable.contentY + flickable.height) {
                flickable.contentY = (item.index + 1) * item.height - flickable.height
            }
        }
    }

    LoggingCategory {
        id: category
        name: "gonnect.qml.chat.UserSelect"
        defaultLogLevel: LoggingCategory.Warning
    }

    Flickable {
        id: flickable
        anchors.fill: parent
        contentHeight: col.implicitHeight
        clip: true
        ScrollBar.vertical: ScrollBar { width: 5 }

        Column {
            id: col

            Repeater {
                id: userRepeater
                model: ChatRoomUsersProxyModel {
                    id: proxyModel

                    ChatRoomUsers {
                        id: usersModel
                    }
                }
                delegate: MenuItem {
                    id: delg

                    required property int index
                    required property string id
                    required property string computedName
                    required property string avatarPath

                    leftPadding: 10
                    rightPadding: 10
                    hoverEnabled: true
                    highlighted: control.selectedIndex === delg.index
                    width: control.maxItemWidth

                    contentItem: Row {
                        spacing: delg.spacing

                        AvatarImage {
                            id: avatarImage
                            size: 36
                            source: delg.avatarPath
                            initials: ViewHelper.initials(delg.computedName)
                            showPresenceStatus: false
                            anchors {
                                verticalCenter: parent.verticalCenter
                            }
                        }

                        Label {
                            text: `${delg.computedName} (${delg.id})`
                            font: delg.font
                            anchors.verticalCenter: parent.verticalCenter
                            color: delg.enabled ? delg.Material.foreground : delg.Material.hintTextColor
                        }
                    }

                    onTriggered: () => control.accepted(delg.id)

                    HoverHandler {
                        id: delgHoveredHandler
                        onHoveredChanged: () => {
                            if (delgHoveredHandler.hovered) {
                                control.selectedIndex = delg.index
                            }
                        }
                    }
                }
            }
        }
    }
}
