pragma ComponentBehavior: Bound

import QtQuick
import QtMultimedia
import base

// VideoOutput with rounded corners (clip only cuts rectangles)
Item {
    id: control

    property real radius: 0
    property color backgroundColor: Theme.backgroundcolor
    readonly property alias videoSink: videoOutput.videoSink

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectFit
    }

    Rectangle {
        id: cornerCover
        color: 'transparent'
        radius: control.radius * 2
        antialiasing: true
        anchors {
            fill: parent
            margins: -control.radius
        }
        border {
            width: control.radius
            color: control.backgroundColor
        }
    }
}
