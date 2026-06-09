pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Popup {
    id: control
    implicitWidth: control.maxItemWidth + 20
    implicitHeight: Math.min(400, col.implicitHeight) + 20

    signal accepted(string emoji)

    property alias filterText: proxyModel.filterText

    property int selectedIndex: -1

    readonly property alias count: emojiRepeater.count

    readonly property int maxItemWidth: {
        let width = 0
        for (let i = 0; i < emojiRepeater.count; ++i) {
            const item = emojiRepeater.itemAt(i)
            width = Math.max(width, item.implicitWidth)
        }
        return width
    }

    function emojiAt(index : int) : string {
        const item = emojiRepeater.itemAt(index)
        if (item) {
            return item.emoji
        }
        console.error(category, `Index ${index} is out of bounds (bounds: [0;${emojiRepeater.count}[)`)
        return ""
    }

    function decrementIndex() {
        let newIndex = control.selectedIndex - 1
        if (newIndex < -1) {
            newIndex = emojiRepeater.count - 1
        }
        control.selectedIndex = newIndex
        control.scrollCurrentSelectionIntoView()
    }

    function incrementIndex() {
        let newIndex = control.selectedIndex + 1
        if (newIndex >= emojiRepeater.count) {
            newIndex = -1
        }
        control.selectedIndex = newIndex
        control.scrollCurrentSelectionIntoView()
    }

    function scrollCurrentSelectionIntoView() {
        const item = emojiRepeater.itemAt(control.selectedIndex)
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
        name: "gonnect.qml.chat.FilteredEmojis"
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
                id: emojiRepeater
                model: EmojiProxyModel {
                    id: proxyModel

                    EmojiModel {}
                }
                delegate: MenuItem {
                    id: delg

                    required property int index
                    required property string emoji
                    required property string label

                    leftPadding: 10
                    rightPadding: 10
                    hoverEnabled: true
                    highlighted: control.selectedIndex === delg.index
                    width: control.maxItemWidth

                    contentItem: Row {
                        spacing: delg.spacing

                        Label {
                            id: emojiIconLabel
                            text: delg.emoji
                            anchors.verticalCenter: parent.verticalCenter
                            horizontalAlignment: Label.AlignHCenter
                            verticalAlignment: Label.AlignVCenter
                            font {
                                family: "Noto Color Emoji"
                                pixelSize: 20
                            }

                            Accessible.ignored: true
                        }

                        Label {
                            text: delg.label
                            font: delg.font
                            anchors.verticalCenter: parent.verticalCenter
                            color: delg.enabled ? delg.Material.foreground : delg.Material.hintTextColor
                        }
                    }

                    onTriggered: () => control.accepted(delg.emoji)

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
