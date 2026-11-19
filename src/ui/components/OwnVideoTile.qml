pragma ComponentBehavior: Bound

import QtQuick
import QtMultimedia
import base

Rectangle {
    id: control

    property bool isActive: false

    radius: Theme.d
    color: Theme.backgroundSecondaryColor
    clip: true

    CameraPreview {
        videoSink: videoOutput.videoSink
        isActive: control.isActive
    }

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        visible: control.isActive
        fillMode: VideoOutput.PreserveAspectFit

        // Show like a mirror
        transform: Scale {
            origin.x: videoOutput.width / 2
            xScale: -1
        }
    }
}
