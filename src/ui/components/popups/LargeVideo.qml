pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control
    implicitWidth: videoPlayer.implicitWidth
    implicitHeight: videoPlayer.implicitHeight

    readonly property bool hidePopupBackground: true

    property alias source: videoPlayer.fileUrl
    property alias fileName: videoPlayer.fileName
    property alias fileSize: videoPlayer.fileSize
    property alias thumbnailFilePath: videoPlayer.thumbnailFileUrl

    VideoPlayer {
        id: videoPlayer
        showFullscreenButton: false
        height: videoPlayer.implicitHeight / videoPlayer.implicitWidth * videoPlayer.width

        anchors {
            centerIn: parent
        }
    }
}
