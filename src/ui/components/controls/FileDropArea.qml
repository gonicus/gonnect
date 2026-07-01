pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import base

Item {
    id: control
    implicitWidth: label.implicitWidth
    implicitHeight: label.implicitHeight

    signal dropAccepted(string url)

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
                label.font.pixelSize: 16
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
            font.pixelSize: 24
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
                       if (dropInArea.isValid(drag)) {
                           drag.accept(Qt.CopyAction)
                       }
                   }

        onDropped: drop => {
                       if (dropInArea.isValid(drop)) {
                           drop.accept(Qt.CopyAction)
                           control.dropAccepted(drop.urls[0].toString())
                       }
                   }

        property bool isInputValid: false
        property string invalidMessage

        function isValid(ev) {

            console.debug(category, "FileDropArea checking validity", "has Urls:", ev.hasUrls, "urls:", ev.urls)

            const bailOut = (msg, logMsg = "") => {
                if (logMsg) {
                    console.debug(category, logMsg)
                }

                dropInArea.isInputValid = false
                dropInArea.invalidMessage = msg
            }

            if (!ev.hasUrls) {
                bailOut(qsTr("Not a file"), "drag event has no urls")
                return false
            }
            if (ev.urls.length !== 1) {
                bailOut(qsTr("Only single file allowed"), `${ev.urls.length} urls in drag event`)
                return false
            }

            const url = ev.urls[0]
            console.debug(category, "file url", url)

            if (!url.toString().startsWith("file://")) {
                bailOut(qsTr("Disallowed type"), "Not a local file url")
                return false
            }
            if (!FileContentHelper.isLocalReadable(url)) {
                bailOut(qsTr("File not readable"), "File not readable")
                return false
            }
            if (FileContentHelper.isLocalDirectory(url)) {
                bailOut(qsTr("Directories are not allowed"), "Url points to a directory, not a file")
                return false
            }
            if (!FileContentHelper.isLocalFile(url)) {
                bailOut(qsTr("Not a file"), "Url does not point to a valid file")
                return false
            }

            dropInArea.isInputValid = true
            return true
        }
    }
}
