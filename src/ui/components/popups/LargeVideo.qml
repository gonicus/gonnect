pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    implicitWidth: videoPlayer.implicitWidth
    implicitHeight: videoPlayer.implicitHeight

    readonly property bool hidePopupBackground: true

    property alias content: videoPlayer.content

    VideoPlayer {
        id: videoPlayer
        showFullscreenButton: false
        height: videoPlayer.implicitHeight / videoPlayer.implicitWidth * videoPlayer.width

        anchors {
            centerIn: parent
        }
    }
}
