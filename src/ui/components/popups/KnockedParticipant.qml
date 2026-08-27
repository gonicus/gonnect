pragma ComponentBehavior: Bound

import QtQuick
import base

Item {
    id: control
    implicitWidth: 360
    implicitHeight: container.implicitHeight

    property var knockModel: []

    signal accepted(string participantId)
    signal rejected(string participantId)

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

        Repeater {
            model: control.knockModel
            delegate: Item {
                id: delg
                height: Math.max(label.height, noButton.height, yesButton.height)
                anchors {
                    left: parent?.left
                    right: parent?.right
                    leftMargin: Theme.d
                    rightMargin: Theme.d
                }

                required property var modelData

                Label {
                    id: label
                    text: qsTr("<b>%1</b> wants to join the conference. Shall that be allowed?").arg(delg.modelData.name)
                    wrapMode: Text.Wrap
                    textFormat: Label.StyledText
                    anchors {
                        left: parent.left
                        right: noButton.left
                        rightMargin: Theme.d
                    }
                }

                Button {
                    id: noButton
                    text: qsTr("No")
                    anchors {
                        right: yesButton.left
                        rightMargin: Theme.d
                    }
                    onClicked: () => control.rejected(delg.modelData.id)
                }

                Button {
                    id: yesButton
                    highlighted: true
                    text: qsTr("Yes")
                    anchors.right: parent.right
                    onClicked: () => control.accepted(delg.modelData.id)
                }
            }
        }
    }
}
