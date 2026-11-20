pragma ComponentBehavior: Bound

import QtQuick
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
        visible: control.isActive
        backgroundColor: control.color

        // Show like a mirror
        transform: Scale {
            origin.x: videoOutput.width / 2
            xScale: -1
        }
    }
}
