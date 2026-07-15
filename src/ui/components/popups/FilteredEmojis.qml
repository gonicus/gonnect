pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Popup {
    id: control
    implicitWidth: 280
    implicitHeight: Math.min(400, listView.contentHeight) + 20

    signal accepted(string emoji)

    property alias filterText: proxyModel.filterText

    property int selectedIndex: -1

    readonly property alias count: listView.count

    function emojiAt(index : int) : string {
        const item = listView.itemAtIndex(index)
        if (item) {
            return item.emoji
        }
        console.error(category, `Index ${index} is out of bounds (bounds: [0;${listView.count}[)`)
        return ""
    }

    function decrementIndex() {
        let newIndex = control.selectedIndex - 1
        if (newIndex < -1) {
            newIndex = listView.count - 1
        }
        control.selectedIndex = newIndex
        control.scrollCurrentSelectionIntoView()
    }

    function incrementIndex() {
        let newIndex = control.selectedIndex + 1
        if (newIndex >= listView.count) {
            newIndex = -1
        }
        control.selectedIndex = newIndex
        control.scrollCurrentSelectionIntoView()
    }

    function scrollCurrentSelectionIntoView() {
        if (control.selectedIndex >= 0) {
            listView.positionViewAtIndex(control.selectedIndex, ListView.Contain)
        }
    }

    LoggingCategory {
        id: category
        name: "gonnect.qml.chat.FilteredEmojis"
        defaultLogLevel: LoggingCategory.Warning
    }

    ListView {
        id: listView
        anchors.fill: parent
        clip: true
        ScrollBar.vertical: ScrollBar { width: 5 }

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
            width: listView.width

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
