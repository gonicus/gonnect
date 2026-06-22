pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    height: slider.height
    implicitWidth: slider.implicitWidth
                   + slider.anchors.rightMargin
                   + volumeLabel.width

    property alias value: slider.value
    readonly property alias labelText: volumeLabel.text

    signal moved

    Slider {
        id: slider
        from: 0
        to: 100
        stepSize: 1
        value: 90
        anchors {
            left: parent.left
            right: volumeLabel.left
            rightMargin: 20
        }

        onMoved: () => control.moved()

        Accessible.role: Accessible.Slider
        Accessible.name: qsTr("Adjust volume")
        Accessible.focusable: true
        Accessible.onIncreaseAction: () => {
            if (slider.value < slider.to) {
                slider.value += slider.stepSize
            }
        }
        Accessible.onDecreaseAction: () => {
            if (slider.value > slider.from) {
                slider.value -= slider.stepSize
            }
        }
    }

    Label {
        id: volumeLabel
        //: Label for showing percentage
        text: qsTr('%1 %').arg(slider.value.toLocaleString(Qt.locale(), "f", 0))
        horizontalAlignment: Label.AlignRight
        width: 40
        anchors {
            right: parent.right
            verticalCenter: slider.verticalCenter
        }

        Accessible.ignored: true
    }
}
