pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    implicitWidth: 600
    implicitHeight: 260

    property alias url: urlLabel.text
    property alias text: textLabel.text

    Column {
        id: container
        spacing: 20
        topPadding: closeButton.height + 20
        anchors {
            left: parent.left
            right: parent.right
        }

        Label {
            id: textLabel
            wrapMode: Label.Wrap
            anchors {
                left: parent.left
                right: parent.right
            }
        }

        Rectangle {
            color: Theme.backgroundOffsetColor
            height: 30
            radius: 4
            anchors {
                left: parent.left
                right: parent.right
            }

            Label {
                id: urlLabel
                elide: Label.ElideRight
                anchors {
                    verticalCenter: parent.verticalCenter
                    left: parent.left
                    right: parent.right
                    leftMargin: 10
                    rightMargin: 10
                }
            }
        }

        Button {
            text: qsTr("Copy and close")
            highlighted: true
            anchors.horizontalCenter: parent.horizontalCenter
            onClicked: () => {
                ClipboardHelper.copyToClipboard(control.url)
                control.StackView.view.popCurrentItem(StackView.Immediate)
            }
        }
    }

    HeaderIconButton {
        id: closeButton
        iconSource: Icons.mobileCloseApp
        anchors {
            top: parent.top
            right: parent.right
        }

        onClicked: () => control.StackView.view.popCurrentItem(StackView.Immediate)
    }
}
