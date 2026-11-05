pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import base

Item {
    id: control
    implicitWidth: label.implicitWidth
    implicitHeight: label.implicitHeight

    signal dropAccepted(list<url> url)
    signal dropRejected(string reason) // TODO: @Markus -> handle this instead of direct reaction in drop zone

    property bool compact: false

    states: [
        State {
            when: control.compact

            PropertyChanges {
                outlineBg.anchors.margins: 5
                outlineBg.border.width: 2
                outlineBg.radius: 5
                label.icon.width: 20
                label.icon.height: 20
                label.font.pixelSize: Theme.fontSizeMedium
            }
        }
    ]

    LoggingCategory {
        id: category
        name: "gonnect.qml.FileDropArea"
        defaultLogLevel: LoggingCategory.Debug
    }

    Item {
        visible: dropInArea.containsDrag
        anchors.fill: parent

        Rectangle {
            anchors.fill: outlineBg
            radius: outlineBg.radius
            color: Theme.backgroundSecondaryColor
            opacity: 0.85
        }

        Rectangle {
            id: outlineBg
            color: 'transparent'
            radius: 20
            border {
                color: Theme.primaryTextColor
                width: 5
            }
            anchors {
                fill: parent
                margins: 20
            }
        }

        IconLabel {
            id: label
            anchors.centerIn: parent
            color: Theme.primaryTextColor
            font.pixelSize: Theme.fontSizeExtraLarge
            text: dropInArea.isInputValid ? qsTr("Send attachment") : dropInArea.invalidMessage
            spacing: 20
            icon {
                color: Theme.primaryTextColor
                width: 36
                height: 36
                source: dropInArea.isInputValid ? Icons.uploadMedia : Icons.dialogCancel
            }
        }
    }

    DropArea {
        id: dropInArea
        anchors.fill: parent
        onEntered: drag => {
                       if (dropInArea.isDroppable(drag)) {
                           drag.accept(Qt.CopyAction)
                       }
                   }

        onExited: {
            dropInArea.isInputValid = false
            dropInArea.invalidMessage = ""
        }

        onDropped: drop => {
            if (!dropInArea.isInputValid) {
                return
            }

            const urls = FileContentHelper.uploadableUrls(dropInArea.resolveUrls(drop))
            const rejectionReason = dropInArea.rejectionReason(urls)

            if (rejectionReason) {
                console.debug(category, "Drop rejected:", rejectionReason)
                dropInArea.isInputValid = false
                dropInArea.invalidMessage = rejectionReason
                control.dropRejected(rejectionReason)
                return
            }

            drop.accept(Qt.CopyAction)
            control.dropAccepted(urls)
        }

        property bool isInputValid: false
        property string invalidMessage
        property list<url> uploadUrls

        function isDroppable(ev) {
            const hasPortalTransfer = ev.formats.includes(FileContentHelper.portalFileTransferMimeType)

            console.debug(category, "FileDropArea checking drag", "has Urls:", ev.hasUrls,
                          "portal file transfer:", hasPortalTransfer)

            if (!ev.hasUrls && !hasPortalTransfer) {
                console.debug(category, "drag event has no urls")
                dropInArea.isInputValid = false
                dropInArea.invalidMessage = qsTr("Not a file")
                return false
            }

            dropInArea.isInputValid = true
            dropInArea.invalidMessage = ""
            return true
        }

        function resolveUrls(ev) {
            if (ev.formats.includes(FileContentHelper.portalFileTransferMimeType)) {
                const token = ev.getDataAsString(FileContentHelper.portalFileTransferMimeType)
                const resolved = FileContentHelper.resolvePortalFileToken(token)

                if (resolved.length) {
                    return resolved
                }
            }

            return ev.urls
        }

        function rejectionReason(urls) {
            if (!urls.length) {
                return qsTr("No valid files")
            }

            for (const url of urls) {
                console.debug(category, "Checking single file url", url)

                if (!url.toString().startsWith("file://")) {
                    return qsTr("Disallowed type")
                }
                if (!FileContentHelper.isLocalReadable(url)) {
                    return qsTr("File not readable")
                }
                if (!FileContentHelper.isLocalDirectory(url) && !FileContentHelper.isLocalFile(url)) {
                    return qsTr("Not a file")
                }
            }

            return ""
        }
    }
}
