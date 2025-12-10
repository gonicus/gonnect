pragma ComponentBehavior: Bound

import QtQuick
import QtWebView
import base

Item {
    id: control

    required property string primaryUrl
    required property string secondaryUrl

    property bool showPrimary: true

    // Segfaults... Needs QTWEBENGINE_DISABLE_SANDBOX=1 and --no-sandbox in contaier??
    WebView {
        id: webView
        url: control.showPrimary ? control.primaryUrl : control.secondaryUrl
        anchors.fill: parent

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
}
