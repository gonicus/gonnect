pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtWebEngine
import base

BaseWidget {
    id: control

    property string primaryUrl
    property string secondaryUrl

    onPrimaryUrlChanged: {
        control.config.set("primaryUrl", control.primaryUrl)
        console.error(control.config.entries())
    }

    onSecondaryUrlChanged: {
        control.config.set("secondaryUrl", control.secondaryUrl)
    }

    property bool showPrimary: true

    Rectangle {
        id: webviewWidget
        parent: control.root
        color: "transparent"
        anchors {
            centerIn: parent
            fill: parent
        }

        CardHeading {
            id: webviewHeading
            visible: true
            text: qsTr("Tracker")
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
            }
        }

        Row {
            id: webviewControl
            spacing: 5
            anchors {
                top: webviewHeading.bottom
                horizontalCenter: parent.horizontalCenter
            }

            Button {
                id: switchButton
                text: control.showPrimary == true ? "1" : "2"
                height: 40

                onClicked: () => {
                    control.showPrimary = !control.showPrimary
                }
            }

            Button {
                id: editButton
                icon.source: Icons.editor
                text: qsTr("Edit")
                height: 40

                onClicked: () => {
                    urlEntryDialog.open()
                }
            }
        }

        /*
            INFO: Segfaults... ttps://chromium.googlesource.com/chromium/src/+/master/base/memory/ref_counted.h#445
            See TEST flags in main.cpp
        */
        WebEngineView {
            id: webView
            anchors {
                top: webviewControl.bottom
                bottom: parent.bottom
                left: parent.left
                right: parent.right

                bottomMargin: 20
                leftMargin: 20
                rightMargin: 20
            }

            url: control.showPrimary ? control.primaryUrl : control.secondaryUrl
            settings {
                autoLoadImages: true
                errorPageEnabled: true
                javascriptEnabled: true
                localStorageEnabled: true
                localContentCanAccessFileUrls: true
            }

            onLoadingChanged: function(loadRequest) {
                if (loadRequest.status === WebEngineView.LoadFailedStatus) {
                    console.error("Page load failed for URL:", loadRequest.url);
                    console.error("Error message:", loadRequest.errorString);
                } else if (loadRequest.status === WebEngineView.LoadSucceededStatus) {
                    console.log("Page loaded successfully!");
                }
            }

            onCertificateError: function(error) {
                console.log("Certificate Error encountered:", error.description);

                // Self hosted stuff
                error.acceptCertificate();

                // Do not jump to default behaviour
                return true;
            }
        }
    }

    Dialog {
        id: urlEntryDialog
        title: qsTr("Set URL's")

        x: parent.width / 2
        y: parent.height / 2

        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

        ColumnLayout {
            anchors.fill: parent
            spacing: 15

            ColumnLayout {
                spacing: 4

                Label {
                    text: qsTr("Primary URL")
                }

                TextField {
                    id: primaryUrlInput
                    Layout.fillWidth: true
                }
            }

            ColumnLayout {
                spacing: 4

                Label {
                    text: qsTr("Secondary URL")
                }

                TextField {
                    id: secondaryUrlInput
                    Layout.fillWidth: true
                }
            }

            Button {
                text: "Done"
                Layout.fillWidth: true
                Layout.topMargin: 10

                onClicked: {
                    control.primaryUrl = primaryUrlInput.text
                    control.secondaryUrl = secondaryUrlInput.text

                    urlEntryDialog.close()
                }
            }
        }
    }
}
