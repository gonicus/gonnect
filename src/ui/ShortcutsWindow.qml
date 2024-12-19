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

    Flickable {
        anchors.fill: parent
        contentHeight: flickableCol.implicitHeight

        ScrollBar.vertical: ScrollBar { width: 10 }


        Column {
            id: flickableCol
            spacing: 10
            topPadding: 20
            bottomPadding: 20
            anchors {
                left: parent.left
                right: parent.right
                leftMargin: 20
                rightMargin: 20
            }

            Repeater {
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

                delegate: Item {
                    id: delg
                    implicitHeight: descriptionLabel.y + descriptionLabel.implicitHeight
                    anchors {
                        left: parent.left
                        right: parent.right
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
                    }

                    Label {
                        id: descriptionLabel
                        wrapMode: Label.Wrap
                        text: delg.description
                        anchors {
                            top: keyLabel.bottom
                            left: parent.left
                            right: parent.right
                            leftMargin: 10
                            rightMargin: 10
                        }
                    }
                }
            }
        }
    }
}
