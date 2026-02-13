pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Window
import QtQuick.Controls.Material
import base

BaseWindow {
    id: control
    objectName: "shortcutsWindow"
    width: 710
    height: 704
    visible: true
    title: qsTr("Shortcuts")
    resizable: false

    minimumWidth: control.width
    minimumHeight: control.height
    maximumWidth: control.width
    maximumHeight: control.height

    component KeyDelegate : Item {
        id: delg
        implicitHeight: descriptionLabel.y + descriptionLabel.implicitHeight
        anchors {
            left: parent?.left
            right: parent?.right
        }

        required property string key
        required property string description

        Label {
            id: keyLabel
            text: delg.key
            font.pixelSize: 16
            font.weight: Font.DemiBold
            anchors {
                top: parent.top
                left: parent.left
            }

            Accessible.role: Accessible.StaticText
            Accessible.name: qsTr("Shortcut key")
            Accessible.description: keyLabel.text
        }

        Label {
            id: descriptionLabel
            wrapMode: Label.Wrap
            text: delg.description
            anchors {
                top: keyLabel.bottom
                left: parent.left
                right: parent.right
                leftMargin: 20
                rightMargin: 10
            }

            Accessible.role: Accessible.StaticText
            Accessible.name: qsTr("Shortcut description")
            Accessible.description: descriptionLabel.text
        }
    }

    Flickable {
        anchors.fill: parent
        contentHeight: flickableCol.implicitHeight

        ScrollBar.vertical: ScrollBar { width: 10 }


        Column {
            id: flickableCol
            spacing: 15
            topPadding: 20
            bottomPadding: 20
            anchors {
                left: parent.left
                right: parent.right
                leftMargin: 20
                rightMargin: 20
            }

            Label {
                id: localShortcutsHeading
                text: qsTr("Local shortcuts (work only when app is focused)")
                font.pixelSize: 16
                font.weight: Font.Medium
                elide: Text.ElideRight
                color: Theme.secondaryTextColor
                visible: SM.globalShortcutsSupported
                anchors {
                    left: parent.left
                    right: parent.right
                }

                Accessible.role: Accessible.StaticText
                Accessible.name: qsTr("Local shortcuts")
                Accessible.description: localShortcutsHeading.text
            }


            KeyDelegate {
                key: qsTr("Ctrl + F")
                description: qsTr("Activates the global search field")
            }
            KeyDelegate {
                key: qsTr("F11")
                description: qsTr("Toggles between fullsceen and normal window mode")
            }
            KeyDelegate {
                key: qsTr("Ctrl + Shift + M")
                description: qsTr("Toggles audio mute")
            }

            Item {
                width: 1
                height: 15
                visible: globalShortcutsHeading.visible
            }

            Label {
                id: globalShortcutsHeading
                text: qsTr("Global shortcuts (work from anywhere)")
                font.pixelSize: 16
                font.weight: Font.Medium
                elide: Text.ElideRight
                color: Theme.secondaryTextColor
                visible: globalShortcutsRepeater.count > 0
                anchors {
                    left: parent.left
                    right: parent.right
                }

                Accessible.role: Accessible.StaticText
                Accessible.name: qsTr("Global shortcuts")
                Accessible.description: globalShortcutsHeading.text
            }

            Repeater {
                id: globalShortcutsRepeater
                model: {
                    const shortcuts = SM.globalShortcuts
                    const result = []
                    for (const key in shortcuts) {
                        if (shortcuts.hasOwnProperty(key)) {
                            const item = shortcuts[key]
                            result.push({
                                            key: item.trigger,
                                            description: item.description
                                        })
                        }
                    }
                    return result
                }

                delegate: KeyDelegate {}
            }
        }
    }
}
