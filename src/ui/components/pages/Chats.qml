pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtWebEngine
import QtWebChannel
import base

Item {
    id: control

    property MatrixConnector attachedData

    states: [
        State {
            when: !!(control.attachedData?.isSecretInitalized && !!control.attachedData?.accessToken)

            PropertyChanges {
                verificationCard.visible: false
                webViewCard.visible: false
                sideBar.visible: true
                chatMainCard.visible: true
            }
        },
        State {
            when: !!control.attachedData?.isSecretInitalized

            PropertyChanges {
                verificationCard.visible: false
                webViewCard.visible: true
            }
        }
    ]

    Connections {
        target: control.attachedData
        function onAccessTokenChanged() {
            matrixView.url = "about:blank"
            matrixView.loadUrl()
        }
    }

    Card {
        id: verificationCard
        anchors {
            fill: parent
            margins: 24
        }

        Column {
            spacing: 20
            anchors {
                left: parent.left
                right: parent.right
                verticalCenter: parent.verticalCenter
            }

            Label {
                text: qsTr("Please enter your recovery key to decrypt messages:")
                anchors.horizontalCenter: parent.horizontalCenter
            }

            TextField {
                id: recoveryKeyTextField
                width: 560
                maximumLength: 12 * 4 + 11 * 3
                placeholderText: qsTr("Recovery key")
                anchors.horizontalCenter: parent.horizontalCenter
                validator: RegularExpressionValidator {
                    regularExpression: /([0-9A-Za-z]{4} ){11}[0-9A-Za-z]{4}/
                }

                Keys.onReturnPressed: () => acceptRecoveryKeyButton.click()
                Keys.onEnterPressed: () => acceptRecoveryKeyButton.click()
            }

            Button {
                id: acceptRecoveryKeyButton
                enabled: recoveryKeyTextField.acceptableInput
                highlighted: true
                text: qsTr("Use key")
                anchors.horizontalCenter: parent.horizontalCenter

                onClicked: () => control.attachedData.handleRecoveryKey(recoveryKeyTextField.text)
            }
        }
    }

    Card {
        id: webViewCard
        visible: false
        anchors {
            fill: parent
            margins: 24
        }

        WebEngineView {
            id: matrixView
            anchors.fill: parent
            settings {
                localStorageEnabled: true
            }
            profile {
                offTheRecord: false
                storageName: "gonnect_matrix"
                httpCacheType: WebEngineProfile.NoCache
            }
            webChannel: WebChannel {
                id: matrixWebChannel

                Component.onCompleted: () => {
                    if (control.attachedData) {
                        matrixWebChannel.registerObject("matrixConnector", control.attachedData)
                        matrixView.loadUrl()
                    }
                }

                readonly property Connections controlConnections: Connections {
                    target: control
                    function onAttachedDataChanged() {
                        if (control.attachedData) {
                            matrixWebChannel.registerObject("matrixConnector", control.attachedData)
                            matrixView.loadUrl()
                        }
                    }
                }
            }

            function loadUrl() {
                matrixView.url = "http://localhost:8080" // "?cacheInhibitor=" + Math.round(Math.random() * 100000)
            }

            onJavaScriptConsoleMessage: (level, message, line, source) => {
                if (level >= 2) {
                    console.error("# " + message + ": " + source + " +" + line)
                } else if (level === 1) {
                    console.warn("# " + message + ": " + source + " +" + line)
                } else {
                    console.log("# " + message + ": " + source + " +" + line)
                }
            }

            onPermissionRequested: (permission) => {
                console.error("#### permission requested", permission.isValid, permission.origin, permission.permissionType, permission.state)
                permission.grant()
            }
        }
    }

    Card {
        id: sideBar
        visible: false
        width: control.width * 1 / 4
        anchors {
            top: parent.top
            left: parent.left
            bottom: parent.bottom
            margins: 24
        }

        MatrixRoomList {
            id: matrixRoomList
            chatProvider: control.attachedData
            clip: true
            anchors {
                fill: parent
                topMargin: 10
                bottomMargin: 10
            }
        }
    }

    Card {
        id: chatMainCard
        visible: false
        anchors {
            top: parent.top
            right: parent.right
            bottom: parent.bottom
            left: sideBar.right
            margins: 24
        }

        MatrixMessageList {
            id: matrixMessageList
            chatRoom: control.attachedData?.chatRoomByRoomId(matrixRoomList.selectedRoomId) ?? null
            clip: true
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                bottom: matrixMessageBox.top
                bottomMargin: 10
                leftMargin: 10
                rightMargin: 10
            }
        }

        MatrixMessageBox {
            id: matrixMessageBox
            enabled: !!matrixRoomList.selectedRoomId
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
                margins: 10
            }

            onSendMessage: () => {
                matrixMessageList.chatRoom.sendMessage(matrixMessageBox.text)
                matrixMessageBox.clear()
            }
        }
    }
}
