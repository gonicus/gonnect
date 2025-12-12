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

    Column {
        id: webviewLayout
        anchors.fill: parent
        spacing: 5

        Button {
            id: toggleButton
            width: 50
            height: 25
            text: control.showPrimary ? "1" : "2"

            onClicked: () => {
                control.showPrimary = !control.showPrimary
            }
        }

        /*
            INFO: Segfaults... ttps://chromium.googlesource.com/chromium/src/+/master/base/memory/ref_counted.h#445
            See TEST flags in main.cpp
        */
        Rectangle {
            id: webviewContainer
            width: parent.width
            height: parent.height - toggleButton.height

            WebEngineView {
                id: webView
                width: parent.width - 25
                height: parent.height - 25
                anchors.centerIn: parent

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
}
