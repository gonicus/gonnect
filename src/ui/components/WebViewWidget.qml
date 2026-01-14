pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtWebEngine
import base

BaseWidget {
    id: control

    property string headerTitle
    property string darkModeUrl
    property string lightModeUrl
    property bool acceptAllCerts

    onAdditionalSettingsLoaded: {
        control.headerTitle = control.config.get("headerTitle")
        control.darkModeUrl = control.config.get("darkModeUrl")
        control.lightModeUrl = control.config.get("lightModeUrl")
        control.acceptAllCerts = control.config.get("acceptAllCerts")
    }

    onAdditionalSettingsUpdated: {
        control.config.set("headerTitle", control.headerTitle)
        control.config.set("darkModeUrl", control.darkModeUrl)
        control.config.set("lightModeUrl", control.lightModeUrl)
        control.config.set("acceptAllCerts", control.acceptAllCerts)
    }

    onCleanupRequested: {
        // INFO: Avoid WebEngineView freezes on deletion, especially with invalid URL's
        webView.stop()
        webView.url = "about:blank"
    }

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
            text: control.headerTitle
            anchors {
                left: parent.left
                right: parent.right
            }
        }

        WebEngineView {
            id: webView
            anchors {
                top: webviewHeading.bottom
                bottom: parent.bottom
                left: parent.left
                right: parent.right

                topMargin: 15
                bottomMargin: 15
                leftMargin: 15
                rightMargin: 15
            }

            url: Theme.isDarkMode ? control.darkModeUrl : control.lightModeUrl
            backgroundColor: Theme.backgroundColor
            settings {
                autoLoadImages: true
                errorPageEnabled: true
                javascriptEnabled: true
                localStorageEnabled: true
                localContentCanAccessFileUrls: true
            }

            onLoadingChanged: function(loadRequest) {
                if (loadRequest.status === WebEngineView.LoadFailedStatus) {
                    console.error("Failed to load page:", loadRequest.url,
                                  ", error message:", loadRequest.errorString)
                }
            }

            onCertificateError: function(error) {
                console.log("Certificate error encountered:", error.description);

                if (control.acceptAllCerts) {
                    // Self hosted stuff
                    error.acceptCertificate()

                    // Do not jump to default behaviour
                    return true
                }

                return false
            }
        }
    }
}
