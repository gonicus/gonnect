pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    implicitWidth: videoPlayer.implicitWidth
    implicitHeight: videoPlayer.implicitHeight

    property alias content: videoPlayer.content

    readonly property bool hidePopupBackground: true
    readonly property real popupMargin: 4 * Theme.d
    readonly property real windowWidth: control.window?.width ?? 1280
    readonly property real windowHeight: control.window?.height ?? 720
    readonly property real cardMaxWidth: control.windowWidth - 2 * control.popupMargin
    readonly property real cardMaxHeight: control.windowHeight - 2 * control.popupMargin
    readonly property real targetVideoHeight: videoPlayer.sourceSize.height > 0
                                              ? Math.min(videoPlayer.sourceSize.height, control.cardMaxHeight - videoPlayer.controlHeight, control.cardMaxWidth / videoPlayer.aspectRatio)
                                              : Math.max(0, control.cardMaxHeight - videoPlayer.controlHeight)

    VideoPlayer {
        id: videoPlayer
        showFullscreenButton: false
        explicitVideoHeight: control.targetVideoHeight
        anchors.centerIn: parent
    }
}
