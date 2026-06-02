pragma ComponentBehavior: Bound

import QtQuick
import base

Item {
    id: control
    implicitHeight: relatedCol.implicitHeight
    implicitWidth: relatedContent.implicitWidth

    property alias isStateUpdate: relatedContent.isStateUpdate
    property alias isText: relatedContent.isText
    property alias isSimpleText: relatedContent.isSimpleText
    property alias isMultiText: relatedContent.isMultiText
    property alias isImage: relatedContent.isImage
    property alias isFile: relatedContent.isFile
    property alias isAudioFile: relatedContent.isAudioFile
    property alias isVideoFile: relatedContent.isVideoFile

    property alias userState: relatedContent.userState
    property alias affectedUserName: relatedContent.affectedUserName
    property alias simpleText: relatedContent.simpleText
    property alias multiText: relatedContent.multiText
    property alias imageUrl: relatedContent.imageUrl
    property alias fileUrl: relatedContent.fileUrl
    property alias fileName: relatedContent.fileName
    property alias fileSize: relatedContent.fileSize
    property alias thumbnailFileUrl: relatedContent.thumbnailFileUrl

    property string nickName

    Rectangle {
        id: relatedBorder
        width: 2
        radius: 1
        color: Theme.backgroundInitials
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }
    }

    Column {
        id: relatedCol
        topPadding: 5
        bottomPadding: 10
        spacing: 5

        anchors {
            top: parent.top
            leftMargin: 10
            left: relatedBorder.right
            right: parent.right
        }

        Label {
            color: Theme.secondaryTextColor
            font.weight: Font.DemiBold
            wrapMode: Label.Wrap
            text: qsTr("Answer to message from %1").arg(control.nickName)
            anchors {
                left: parent.left
                right: parent.right
            }

            Accessible.ignored: true
        }

        ChatMessageListItemContent {
            id: relatedContent
            messageLabel.color: Theme.secondaryTextColor
            anchors {
                left: parent.left
                right: parent.right
            }
        }
    }
}
