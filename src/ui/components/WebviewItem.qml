pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtWebEngine
import base

Item {
    id: control

    required property string primaryUrl
    required property string secondaryUrl

    property bool showPrimary: true

    /*
        INFO: Segfaults... ttps://chromium.googlesource.com/chromium/src/+/master/base/memory/ref_counted.h#445
        See TEST flags in main.cpp
    */
    Rectangle {
        id: webviewContainer
        color: "transparent"
        anchors.fill: parent

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

        Button {
            id: toggleButton
            text: control.showPrimary == true ? "1" : "2"
            height: 30
            anchors {
                top: webviewHeading.bottom
                horizontalCenter: parent.horizontalCenter
            }

            onClicked: () => {
                control.showPrimary = !control.showPrimary
            }
        }

        WebEngineView {
            id: webView
            anchors {
                top: toggleButton.bottom
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
}
