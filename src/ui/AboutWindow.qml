pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Window
import QtQuick.Controls.Material
import base

BaseWindow {
    id: control
    objectName: "aboutWindow"
    title: qsTr("About")
    width: 600
    height: 380
    visible: true
    resizable: false
    showMinimizeButton: false
    showMaximizeButton: false

    minimumWidth: control.width
    minimumHeight: control.height
    maximumWidth: control.width
    maximumHeight: control.height

    Column {
        spacing: 20
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            margins: 20
        }

        Settings {
            id: genericSettings
            location: ViewHelper.userConfigPath
            category: "generic"

            property string homePageURL: "https://gonnect.gonicus.de"
            property string issueTrackerURL: "https://github.com/gonicus/gonnect/issues"
            property string documentationURL: "https://docs.gonicus.de/"
        }

        Image {
            id: img
            source: "qrc:/icons/gonnect.svg"
            sourceSize.width: 128
            sourceSize.height: 128
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Label {
            text: "GOnnect - your friendly VoIP client that connects people"
            font.weight: Font.DemiBold
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Item {
            id: versionRow
            implicitHeight: appLabel.implicitHeight
            anchors.horizontalCenter: parent.horizontalCenter
            width: appLabel.width + clipboardButton.width

            Label {
                id: appLabel
                text: qsTr("Version: v%1").arg(ViewHelper.appVersion())
            }

            ClipboardButton {
                id: clipboardButton
                text: ViewHelper.appVersion()
                anchors {
                    left: appLabel.right
                    leftMargin: 20
                    verticalCenter: parent.verticalCenter
                }
            }
        }

        Rectangle {
            height: 1
            color: Theme.borderColor
            anchors {
                left: parent.left
                right: parent.right
            }
        }


        Row {
            spacing: 20
            anchors.horizontalCenter: parent.horizontalCenter

            Label {
                text: '<a href="%1">%2</a>'
                    .replace('%1', genericSettings.homePageURL)
                    .replace('%2', qsTr('Homepage'))
                onLinkActivated: link => Qt.openUrlExternally(link)

                HoverHandler {
                    enabled: parent.hoveredLink
                    cursorShape: Qt.PointingHandCursor
                }
            }
            Label {
                text: '<a href="%1">%2</a>'
                    .replace('%1', genericSettings.issueTrackerURL)
                    .replace('%2', qsTr('Bug Tracker'))
                onLinkActivated: link => Qt.openUrlExternally(link)

                HoverHandler {
                    enabled: parent.hoveredLink
                    cursorShape: Qt.PointingHandCursor
                }
            }
            Label {
                text: '<a href="%1">%2</a>'
                    .replace('%1', genericSettings.documentationURL)
                    .replace('%2', qsTr('Documentation'))
                onLinkActivated: link => Qt.openUrlExternally(link)

                HoverHandler {
                    enabled: parent.hoveredLink
                    cursorShape: Qt.PointingHandCursor
                }
            }
        }
    }
}
