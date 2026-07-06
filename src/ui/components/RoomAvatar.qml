pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls.impl
import QtQuick.Controls
import base


AvatarImage {
    id: control
    size: 100
    showPresenceStatus: true

    property IChatProvider chatProvider

    indicatorComponent: Item {
        id: editIndicator

        Rectangle {
            id: indicatorBg
            anchors.fill: parent
            radius: indicatorBg.width / 2
            color: editIndicatorHoverHandler.hovered ? Theme.backgroundOffsetHoveredColor : Theme.backgroundOffsetColor
        }

        IconLabel {
            anchors.fill: parent

            icon {
                source: Icons.editor
                color: editIndicatorHoverHandler.hovered ? Theme.primaryTextColor : Theme.secondaryTextColor
            }
        }

        HoverHandler {
            id: editIndicatorHoverHandler
            cursorShape: Qt.PointingHandCursor
        }

        TapHandler {
            onTapped: () => indicatorContextMenu.createObject(editIndicator).popup()
        }
    }

    FileDialog {
        id: uploadAvatarDialog
        onAccepted: () => control.source = control.chatProvider.uploadFile(uploadAvatarDialog.selectedFile)
        nameFilters: FileHelper.imageFileSelectors()
    }

    Component {
        id: indicatorContextMenu

        Menu {
            id: indicatorMenu
            onClosed: () => indicatorMenu.destroy()

            Action {
                text: qsTr("Upload file")
                icon.source: Icons.uploadMedia
                onTriggered: () => uploadAvatarDialog.open()
            }
            Action {
                text: qsTr("Remove")
                enabled: !!control.source.toString()
                icon.source: Icons.editDelete
                onTriggered: () => control.source = ""
            }
        }
    }
}

