pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import base

Item {
    id: control

    property IChatProvider chatProvider

    onChatProviderChanged: () => control.updateState()
    Component.onCompleted: () => control.updateState()

    onStateChanged: () => {
        if (control.state !== "UNVERIFIED") {
            internal.verificationError = ""
        }
    }

    QtObject {
        id: internal

        property string verificationError
        property list<int> availableMethods
        property crossSigningSecret secret
    }

    function updateState() {
        const provider = control.chatProvider

        if (!provider) {
            control.state = "NO_CHAT_PROVIDER"
            return
        }
        if (provider.isDeviceVerified && !provider.isInVerificationProcess) {
            control.state = "FINISHED"
        }
        if (!provider.isDeviceVerified && !provider.isInVerificationProcess) {
            control.state = "UNVERIFIED"
        }
    }

    Connections {
        target: control.chatProvider

        function onIsInVerificationProcessChanged() {
            if (!control.chatProvider?.isInVerificationProcess) {
                control.state = "UNVERIFIED"
            }
        }
        function onIsDeviceVerifiedChanged() {
            control.state = control.chatProvider?.isDeviceVerified ? "FINISHED" : "UNVERIFIED"
        }
        function onVerificationError(error : string) {
            internal.verificationError = error
            control.state = "UNVERIFIED"
        }
        function onCrossSigningMethodSelectRequired(methods : list<int>) {
            internal.availableMethods = methods
            control.state = "CROSS_SIGNING_METHOD_SELECT"
        }
        function onCrossSigningAcceptRequired(secret : crossSigningSecret) {
            internal.secret = secret
            control.state = "CROSS_SIGNING_CODE"
        }
    }

    states: [

        State {
            name: "NO_CHAT_PROVIDER"

            PropertyChanges {
                noChatProviderScreen.visible: true
            }
        },
        State {
            name : "UNVERIFIED"

            PropertyChanges {
                unverifiedIdleScreen.visible: true
            }
        },
        State {
            name : "WAITING_FOR_RESPONSE"

            PropertyChanges {
                waitingForResponseScreen.visible: true
            }
        },
        State {
            name : "RECOVERY_KEY_INPUT"

            PropertyChanges {
                recoveryKeyInputScreen.visible: true
            }
        },
        State {
            name : "CROSS_SIGNING_METHOD_SELECT"

            PropertyChanges {
                crossSigningMethodSelectScreen.visible: true
            }
        },
        State {
            name : "CROSS_SIGNING_CODE"

            PropertyChanges {
                crossSigningCodeScreen.visible: true
            }
        },
        State {
            name : "FINISHED"

            PropertyChanges {
                finishedSigningCodeScreen.visible: true
            }
        }
    ]

    Item {
        id: noChatProviderScreen
        visible: false
        anchors.fill: parent

        Label {
            text: qsTr("Waiting for chat provider...")
            font.pixelSize: 22
            anchors.centerIn: parent
        }
    }

    Item {
        id: unverifiedIdleScreen
        visible: false
        anchors.fill: parent

        Column {
            spacing: 24
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
                right: parent.right
                leftMargin: 24
                rightMargin: 24
            }

            Label {
                text: qsTr("Your device is currently not verified. Please choose one of the following methods to verify it.")
                font.pixelSize: 22
                wrapMode: Text.Wrap
                horizontalAlignment: Label.AlignHCenter
                anchors {
                    left: parent.left
                    right: parent.right
                }
            }

            Button {
                id: recoveryKeyMethodButton
                visible: control.chatProvider?.isRecoveryKeyVerificationAvailable ?? false
                text: qsTr("Recovery key")
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: () => control.state = "RECOVERY_KEY_INPUT"
            }

            Button {
                id: crossSigningMethodButton
                visible: control.chatProvider?.isCrossSigningVerificationAvailable ?? false
                text: qsTr("Cross signing")
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: () => {
                    control.chatProvider?.requestCrossSigningStart()
                    control.state = "WAITING_FOR_RESPONSE"
                }
            }
        }
    }

    Item {
        id: waitingForResponseScreen
        visible: false
        anchors.fill: parent

        BusyIndicator {
            running: waitingForResponseScreen.visible
            anchors {
                horizontalCenter: parent.horizontalCenter
                bottom: parent.verticalCenter
                bottomMargin: 24
            }
        }

        Label {
            id: waitingForResponseLabel
            text: qsTr("Waiting for response...")
            font.pixelSize: 22
            anchors {
                horizontalCenter: parent.horizontalCenter
                top: parent.verticalCenter
                topMargin: 24
            }
        }

        Button {
            text: qsTr("Cancel")
            anchors {
                top: waitingForResponseLabel.bottom
                topMargin: 24
                horizontalCenter: parent.horizontalCenter
            }
            onClicked: () => {
                control.state = "UNVERIFIED"
                control.chatProvider.requestVerificationAbort()
            }
        }
    }

    Item {
        id: recoveryKeyInputScreen
        visible: false
        anchors.fill: parent

        Column {
            spacing: 24
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
                right: parent.right
                leftMargin: 24
                rightMargin: 24
            }

            Label {
                text: qsTr("Please enter your recovery key:")
                font.pixelSize: 22
                wrapMode: Text.Wrap
                horizontalAlignment: Label.AlignHCenter
                anchors {
                    left: parent.left
                    right: parent.right
                }
            }

            TextField {
                id: recoveryKeyInputField
                placeholderText: qsTr("Recovery key")
                anchors {
                    left: parent.left
                    right: parent.right
                }

                onAccepted: recoveryKeyEnteredButton.click()
            }

            Button {
                id: recoveryKeyEnteredButton
                text: qsTr("Verify")
                highlighted: true
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: () => {
                    const key = recoveryKeyInputField.text.trim()
                    control.state = "WAITING_FOR_RESPONSE"
                    control.chatProvider.requestRecoveryKeyVerification(key)
                }
            }

            Button {
                text: qsTr("Cancel")
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: () => {
                    control.updateState()
                }
            }
        }
    }

    Item {
        id: crossSigningMethodSelectScreen
        visible: false
        anchors.fill: parent

        Column {
            spacing: 24
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
                right: parent.right
                leftMargin: 24
                rightMargin: 24
            }

            Label {
                text: qsTr("Please choose one of the following cross-signing methods:")
                font.pixelSize: 22
                wrapMode: Text.Wrap
                horizontalAlignment: Label.AlignHCenter
                anchors {
                    left: parent.left
                    right: parent.right
                }
            }

            Repeater {
                model: internal.availableMethods
                delegate: Button {
                    id: methodDelg
                    text: EnumTranslation.crossSigningMethod(methodDelg.modelData)
                    anchors.horizontalCenter: parent.horizontalCenter
                    onClicked: () => {
                        control.state = "WAITING_FOR_RESPONSE"
                        control.chatProvider.selectCrossSigningMethod(methodDelg.modelData)
                    }

                    required property int modelData
                }
            }

            Button {
                text: qsTr("Cancel")
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: () => {
                    control.state = "UNVERIFIED"
                    control.chatProvider.requestVerificationAbort()
                }
            }
        }
    }

    Item {
        id: crossSigningCodeScreen
        visible: false
        anchors.fill: parent

        Column {
            spacing: 24
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
                right: parent.right
                leftMargin: 24
                rightMargin: 24
            }

            Label {
                text: qsTr("Please verify the code:")
                font.pixelSize: 22
                wrapMode: Text.Wrap
                horizontalAlignment: Label.AlignHCenter
                anchors {
                    left: parent.left
                    right: parent.right
                }
            }

            Label {
                wrapMode: Text.Wrap
                horizontalAlignment: Label.AlignHCenter
                visible: internal.secret?.method() === CrossSigningSecret.CrossSigningMethod.SasString
                text: internal.secret?.stringSecret() ?? ""
                font {
                    family: "Monospace"
                    pixelSize: 42
                }
                anchors {
                    left: parent.left
                    right: parent.right
                }
            }

            Row {
                visible: internal.secret?.method() === CrossSigningSecret.CrossSigningMethod.SasSymbol
                anchors.horizontalCenter: parent.horizontalCenter
                height: 80
                spacing: 10

                Repeater {
                    model: internal.secret?.symbolSequence()
                    delegate: Item {
                        id: symbolDelg
                        width: 100
                        anchors {
                            top: parent?.top
                            bottom: parent?.bottom
                        }

                        required property crossSigningSymbol modelData

                        Label {
                            text: symbolDelg.modelData.symbol()
                            font.pixelSize: 36
                            horizontalAlignment: Label.AlignHCenter
                            anchors {
                                top: parent.top
                                left: parent.left
                                right: parent.right
                            }
                        }

                        Label {
                            text: symbolDelg.modelData.description()
                            color: Theme.secondaryTextColor
                            wrapMode: Label.Wrap
                            maximumLineCount: 2
                            elide: Label.ElideRight
                            horizontalAlignment: Label.AlignHCenter
                            anchors {
                                bottom: parent.bottom
                                left: parent.left
                                right: parent.right
                            }
                        }
                    }
                }
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 24
                height: codeRejectButton.implicitHeight

                Button {
                    id: codeRejectButton
                    text: qsTr("Reject")
                    onClicked: () => {
                        control.state = "UNVERIFIED"
                        control.chatProvider.requestVerificationAbort()
                    }
                }

                Button {
                    id: codeAcceptButton
                    text: qsTr("Accept")
                    onClicked: () => {
                        control.state = "WAITING_FOR_RESPONSE"
                        control.chatProvider.acceptVerification()
                    }
                }
            }
        }
    }

    Item {
        id: finishedSigningCodeScreen
        visible: false
        anchors.fill: parent

        Label {
            text: qsTr("Your device has successfully been verified!")
            font.pixelSize: 22
            anchors.centerIn: parent
        }
    }
}
