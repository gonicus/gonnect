pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import base

Rectangle {
    id: control

    property bool isActive: false

    radius: Theme.d
    color: Theme.backgroundSecondaryColor

    CameraPreview {
        videoSink: videoOutput.videoSink
        isActive: control.isActive
    }

    RoundedVideoOutput {
        id: videoOutput
        anchors.fill: parent
        radius: control.radius
        visible: control.isActive && VideoManager.cameraError === ""
        backgroundColor: control.color

        // Show like a mirror
        transform: Scale {
            origin.x: videoOutput.width / 2
            xScale: -1
        }
    }

    Label {
        id: cameraErrorLabel
        visible: control.isActive && VideoManager.cameraError !== ""
        color: Theme.secondaryTextColor
        font.pixelSize: Theme.fontSizeSmall
        wrapMode: Text.Wrap
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("Camera not available")
              + "\n"
              + VideoManager.cameraError
              + "\n"
              + qsTr("Switch the camera off and on again to retry.")
        anchors {
            left: parent.left
            right: parent.right
            verticalCenter: parent.verticalCenter
            margins: Theme.d * 2
        }
    }
}
