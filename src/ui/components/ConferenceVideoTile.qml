pragma ComponentBehavior: Bound

import QtQuick
import base

Rectangle {
    id: control

    required property ConferenceVideoSource source
    required property bool isMuted

    implicitHeight: Theme.d * 18
    implicitWidth: Math.round(control.implicitHeight * 16 / 9)
    radius: Theme.d
    color: Theme.backgroundSecondaryColor

    ConferenceVideoOutput {
        source: control.source
        videoSink: videoOutput.videoSink
        displayedHeight: control.isMuted ? 0 : Math.round(videoOutput.height * Screen.devicePixelRatio)
    }

    RoundedVideoOutput {
        id: videoOutput
        backgroundColor: control.color
        anchors.fill: parent
        radius: control.radius
        visible: !control.isMuted
    }
}
