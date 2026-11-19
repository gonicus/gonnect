pragma ComponentBehavior: Bound

import QtQuick
import QtMultimedia
import base

Rectangle {
    id: control

    required property ConferenceVideoSource source
    required property bool isMuted

    implicitHeight: Theme.d * 18
    implicitWidth: Math.round(control.implicitHeight * 16 / 9)
    radius: Theme.d
    color: Theme.backgroundSecondaryColor
    clip: true

    ConferenceVideoOutput {
        source: control.source
        videoSink: videoOutput.videoSink
        displayedHeight: control.isMuted ? 0 : Math.round(videoOutput.height)
    }

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        visible: !control.isMuted
        fillMode: VideoOutput.PreserveAspectFit
    }
}
