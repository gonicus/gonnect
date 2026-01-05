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

    onAdditionalSettingsLoaded: {
        control.primaryUrl = control.config.get("primaryUrl")
        control.secondaryUrl = control.config.get("secondaryUrl")
    }

    onPrimaryUrlChanged: {
        control.config.set("primaryUrl", control.primaryUrl)
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
                icon.source: Icons.viewRefresh
                text: qsTr("Switch")
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
                    const item = webviewSettingsComponent.createObject(control, {})
                    item.show()
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

    Component {
        id: webviewSettingsComponent

        BaseWindow {
            id: webviewSettings
            objectName: "webviewSettingsWindow"
            title: qsTr("Set URL's")
            width: 600
            height: 340
            visible: true
            resizable: false
            showMinimizeButton: false
            showMaximizeButton: false

            minimumWidth: webviewSettings.width
            minimumHeight: webviewSettings.height
            maximumWidth: webviewSettings.width
            maximumHeight: webviewSettings.height

            ColumnLayout {
                id: mainLayout
                anchors {
                    fill: parent
                    margins: 20
                }
                spacing: 15

                ColumnLayout {
                    id: primaryUrlLayout
                    spacing: 4

                    Label {
                        id: primaryUrlLabel
                        text: qsTr("Primary URL")
                    }

                    TextField {
                        id: primaryUrlInput
                        Layout.fillWidth: true
                    }
                }

                ColumnLayout {
                    id: secondaryUrlLayout
                    spacing: 4

                    Label {
                        id: secondaryUrlLabel
                        text: qsTr("Secondary URL")
                    }

                    TextField {
                        id: secondaryUrlInput
                        Layout.fillWidth: true
                    }
                }

                RowLayout {
                    id: pageButtons
                    spacing: 10
                    Layout.fillWidth: true
                    layoutDirection: Qt.RightToLeft
                    Layout.alignment: Qt.AlignRight | Qt.AlignBottom

                    Button {
                        id: pageCancel
                        text: qsTr("Cancel")

                        onClicked: () => webviewSettings.close()
                    }

                    Button {
                        id: pageConfirm
                        highlighted: true
                        icon.source: Icons.checkbox
                        text: qsTr("Done")

                        onClicked: {
                            control.primaryUrl = primaryUrlInput.text
                            control.secondaryUrl = secondaryUrlInput.text

                            webviewSettings.close()
                        }
                    }
                }
            }
        }
    }
}
