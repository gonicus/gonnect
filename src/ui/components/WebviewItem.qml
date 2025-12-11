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
        spacing: 10

        Button {
            id: toggleButton
            text: control.showPrimary ? "1" : "2"

            onClicked: () => {
                control.showPrimary = !control.showPrimary
            }
        }

        /*
            INFO: Segfaults... ttps://chromium.googlesource.com/chromium/src/+/master/base/memory/ref_counted.h#445
            See TEST flags in main.cpp

            TODO: "Cannot find any Pyroscope data source! Please add and configure a Pyroscope data source to your Grafana instance."
        */
        WebEngineView {
            id: webView
            url: control.showPrimary ? control.primaryUrl : control.secondaryUrl

            onLoadingChanged: function(loadRequest) {
                if (loadRequest.errorString) {
                    console.error(loadRequest.errorString)
                }
            }

            onCertificateError: function(error) {
                console.log("Certificate Error encountered:", error.description);

                // Self hosted stuff
                error.acceptCertificate();

                // Do not jump to default behaviour
                return true;
            }

            Component.onCompleted: {
                webView.settings.javaScriptEnabled = Qt.Checked
                webView.settings.localStorageEnabled = Qt.Checked
                webView.settings.allowFileAccess = Qt.Checked
                webView.settings.localContentCanAccessFileUrls = Qt.Checked
            }
        }
    }
}
