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
            text: dropInArea.isInputValid ? qsTr("Send attachment") : qsTr("Only (single) files allowed")
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

                       console.debug(category, "FileDropArea entered", "has Urls?", drag.hasUrls, "urls", drag.urls)

                       let valid = drag.hasUrls && drag.urls.length === 1
                       if (valid) {
                           const url = drag.urls[0].toString()
                           valid = url.startsWith("file://")

                           console.debug(category, "has file url", url)
                       }
                       if (valid) {
                           valid = FileContentHelper.isLocalFileAndNotDirectory(drag.urls[0])
                           console.debug(category, "is local and not directory", valid)
                       }

                       dropInArea.isInputValid = valid

                       if (valid) {
                           drag.accept(Qt.CopyAction)
                       }
                   }

        onDropped: drop => {
                       let valid = drop.hasUrls && drop.urls.length === 1
                       let url = ""
                       if (valid) {
                           url = drop.urls[0].toString()
                           valid = url.startsWith("file://")
                       }
                       if (valid) {
                           valid = FileContentHelper.isLocalFileAndNotDirectory(url)
                       }

                       if (valid) {
                           drop.accept(Qt.CopyAction)
                           control.dropAccepted(url)
                       }
                   }

        property bool isInputValid: false
    }
}
