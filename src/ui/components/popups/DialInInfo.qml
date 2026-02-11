pragma ComponentBehavior: Bound

import QtQuick
import base

Item {
    id: control
    implicitWidth: 360
    implicitHeight: container.implicitHeight

    property var numbers
    property string code

    function close() {
        ViewHelper.topDrawer.loader.sourceComponent = undefined
    }

    Column {
        id: container
        topPadding: 20
        spacing: 20
        anchors {
            left: parent.left
            right: parent.right
            verticalCenter: parent.verticalCenter
        }

        Label {
            text: qsTr("Call one of the phone numbers below and use this code for authentication.")
            wrapMode: Text.Wrap
            anchors {
                left: parent.left
                right: parent.right
            }
        }

        Rectangle {
            radius: 8
            height: 50
            color: Theme.backgroundSecondaryColor
            anchors {
                left: parent.left
                right: parent.right
            }

            Label {
                text: control.code.replace(/(.{3})/g, '$1 ')
                anchors.centerIn: parent
                font {
                    pixelSize: 24
                    weight: Font.Medium
                    letterSpacing: 1.5
                }
            }

            ClipboardButton {
                text: control.code
                anchors {
                    verticalCenter: parent.verticalCenter
                    right: parent.right
                    margins: 10
                }
            }
        }

        Repeater {
            model: control.numbers ? Object.keys(control.numbers) : []
            delegate: Item {
                id: delg

                required property string modelData
                readonly property list<string> numbers: control.numbers[delg.modelData]

                implicitHeight: numbersCol.implicitHeight
                anchors {
                    left: parent?.left
                    right: parent?.right
                }

                Label {
                    text: delg.modelData
                    elide: Text.ElideRight
                    font.weight: Font.Medium
                    anchors {
                        top: parent.top
                        left: parent.left
                        right: parent.horizontalCenter
                        rightMargin: 20
                    }
                }

                Column {
                    id: numbersCol
                    spacing: 10
                    anchors {
                        top: parent.top
                        left: parent.horizontalCenter
                        right: parent.right
                    }

                    Repeater {
                        model: delg.numbers
                        delegate: Label {
                            id: numberDelg
                            text: numberDelg.modelData
                            elide: Text.ElideRight
                            anchors {
                                left: parent?.left
                                right: parent?.right
                            }

                            required property string modelData
                        }
                    }
                }
            }
        }

        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Close")
            onClicked: () => control.close()
        }
    }
}
