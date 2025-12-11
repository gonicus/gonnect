pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtWebView
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

        // Segfaults... Needs QTWEBENGINE_DISABLE_SANDBOX=1 and --no-sandbox in contaier??
        // https://chromium.googlesource.com/chromium/src/+/master/base/memory/ref_counted.h#445
        /*
        WebView {
            id: webView
            url: control.showPrimary ? control.primaryUrl : control.secondaryUrl

            onLoadingChanged: function(loadRequest) {
                if (loadRequest.errorString) {
                    console.error(loadRequest.errorString)
                }
            }

            Component.onCompleted: {
                webView.settings.javaScriptEnabled = Qt.Checked
                webView.settings.localStorageEnabled = Qt.Checked
                webView.settings.allowFileAccess = Qt.Checked
                webView.settings.localContentCanAccessFileUrls = Qt.Checked
            }
        }
        */
    }
}
