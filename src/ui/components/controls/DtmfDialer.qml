pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import base

Item {
    id: control
    implicitWidth: 410
    implicitHeight: 554

    signal buttonPressed(string button)
    signal dialed(string button)

    component DialButton : Item {
        id: dialButton
        width: 100
        height: 100

        property alias value: buttonLabel.text

        function clicked() {
            control.dialed(dialButton.value)
        }

        Rectangle {
            id: buttonBackground
            color: Theme.borderColor
            radius: buttonBackground.width / 2
            width: dialButton.width - 20
            height: dialButton.height - 20
            anchors.centerIn: parent
        }

        Label {
            id: buttonLabel
            anchors.centerIn: parent
        }

        Connections {
            target: control
            function onButtonPressed(button) {
                if (button === dialButton.value) {
                    dialButton.clicked()
                }
            }
        }

        TapHandler {
            onTapped: () => dialButton.clicked()
        }
    }

    GridLayout {
        columns: 3
        focus: true
        anchors.centerIn: parent

        Keys.onPressed: event => {
            control.buttonPressed(event.text)
        }

        Repeater {
            model: 9
            delegate: DialButton {
                id: delg
                value: delg.index + 1
                required property int index
            }
        }

        DialButton {
            value: '*'
        }
        DialButton {
            value: '0'
        }
        DialButton {
            value: '#'
        }
    }
}
