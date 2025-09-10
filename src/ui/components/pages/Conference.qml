pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtWebEngine
import QtWebChannel
import base

Item {
    id: control

    QtObject {
        id: internal

        property Button authButton
    }

    states: [
        State {
            when: callSideBar.extended

            PropertyChanges {
                verticalDragbarDummyDragHandler.enabled: true
                verticalDragbarDummyHoverHandler.enabled: true
                verticalDragbarDummy.x: 3/4 * control.width
            }

            AnchorChanges {
                target: verticalDragbarDummy
                anchors.right: undefined
            }

            AnchorChanges {
                target: callListCard
                anchors.left: verticalDragbarDummy.right
            }
        }
    ]

    function startConference(meetingId : string, displayName : string, startFlags : int, callHistoryItem : variant) {
        confConn.setCallHistoryItem(callHistoryItem)

        if (!AuthManager.isJitsiAuthRequired || AuthManager.isJitsiRoomAuthenticated(meetingId)) {
            confConn.joinConference(meetingId, displayName, startFlags)
        } else {
            authConn.enabled = true
            authConn.displayName = displayName
            authConn.startFlags = startFlags
            AuthManager.authenticateJitsiRoom(meetingId)
        }
    }

    Connections {
        id: authConn
        enabled: false
        target: AuthManager

        property int startFlags
        property string displayName

        function onJitsiRoomAuthenticated(roomName : string) {
            authConn.enabled = false
            confConn.joinConference(roomName, authConn.displayName, authConn.startFlags)
        }
    }

    readonly property IConferenceConnector iConferenceConnector: JitsiConnector {
        id: confConn
        WebChannel.id: "jitsiConn"

        function onIsInConferenceChanged() {
            if (!confConn.isInConference) {
                internal.authButton.enabled = true
            }
        }
    }

    Loader {
        id: contentLoader
        sourceComponent: {
            if (!AuthManager.isAuthManagerInitialized) {
                return undefined
            }
            if (confConn.isInConference) {
                return jitsiViewComponent
            }
            if (AuthManager.isJitsiAuthRequired && AuthManager.isWaitingForAuth) {
                return waitingForAuthComponent
            }
            if (!AuthManager.isJitsiAuthRequired || AuthManager.isOAuthAuthenticated) {
                return loggedInComponent
            }
            return loginComponent
        }

        anchors {
            top: parent.top
            bottom: parent.bottom
            left: parent.left
            right: callListCard.visible ? verticalDragbarDummy.left : parent.right
            rightMargin: callListCard.visible ? 0 : 24
        }
    }

    Component {
        id: loginComponent

        Item {
            Column {
                spacing: 20
                anchors {
                    left: parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                }

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    height: roomTextField.implicitHeight
                    spacing: 20

                    Label {
                        text: qsTr("Room name:")
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    TextField {
                        id: roomTextField
                        width: 300
                        text: RandomRoomNameGenerator.randomJitsiRoomName()
                    }
                }

                Button {
                    id: authButton
                    text: qsTr("Authenticate")
                    anchors.horizontalCenter: parent.horizontalCenter
                    onClicked: () => {
                        authButton.enabled = false
                        control.startConference(roomTextField.text.trim())
                    }

                    Component.onCompleted: () => {
                        internal.authButton = authButton
                    }
                }
            }
        }
    }


    Component {
        id: loggedInComponent

        Item {
            Column {
                spacing: 20
                anchors {
                    left: parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                }

                // Label {
                //     anchors.horizontalCenter: parent.horizontalCenter
                //     text: `Join "${AuthManager.authenticatedJitsiRoom}"`
                // }


                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    height: roomTextField2.implicitHeight
                    spacing: 20

                    Label {
                        text: qsTr("Room name:")
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    TextField {
                        id: roomTextField2
                        width: 300
                        // text: AuthManager.authenticatedJitsiRoom
                    }
                }

                Button {
                    id: joinRoomButton
                    text: 'Join Room'
                    anchors.horizontalCenter: parent.horizontalCenter

                    onClicked: () => control.startConference(roomTextField2.text.trim())
                }
            }
        }
    }

    Component {
        id: waitingForAuthComponent

        Item {
            Label {
                anchors.centerIn: parent
                text: qsTr("Please authenticate in the opened browser window...")
            }
        }
    }

    Component {
        id: jitsiViewComponent

        Item {
            Card {
                id: callMainCard
                anchors {
                    fill: callMainCard.parent

                    leftMargin: 24
                    bottomMargin: 15
                }

                ConferenceButtonBar {
                    id: topBar
                    height: topBar.implicitHeight
                    enabled: true
                    isOnHold: confConn.isOnHold
                    isMuted: confConn.isAudioMuted
                    isVideoMuted: confConn.isVideoMuted
                    videoMuteButtonVisible: confConn.isVideoAvailable
                    isSharingScreen: confConn.isSharingScreen
                    isTileView: confConn.isTileView
                    isHandRaised: confConn.isHandRaised
                    iConferenceConnector: confConn
                    anchors {
                        left: topBar.parent.left
                        right: topBar.parent.right
                        top: topBar.parent.top
                    }


                    onSetOnHold: () => confConn.toggleHold()
                    onSetAudioMuted: (value) => confConn.setAudioMuted(value)
                    onSetVideoMuted: (value) => confConn.setVideoMuted(value)
                    onSetScreenShare: (value) => confConn.setScreenShare(value)
                    onSetTileView: (value) => confConn.setTileView(value)
                    onSetRaiseHand: (value) => confConn.setHandRaised(value)

                    onShowVirtualBackgroundDialog: () => confConn.showVirtualBackgroundDialog()
                    onOpenSetPasswordDialog: () => ViewHelper.topDrawer.loader.sourceComponent = setPasswordItemComponent
                    onOpenVideoQualityDialog: () => ViewHelper.topDrawer.loader.sourceComponent = videoQualityComponent
                    onHangup: () => confConn.leaveConference()
                    onFinishForAll: () => confConn.terminateConference()
                }

                WebEngineView {
                    id: jitsiView
                    settings {
                        localStorageEnabled: true
                        screenCaptureEnabled: true
                    }
                    profile {
                        offTheRecord: false
                        storageName: "gonnect_jitsi_meet"
                    }
                    webChannel: WebChannel {
                        registeredObjects: [ confConn ]
                    }
                    anchors {
                        top: topBar.bottom
                        bottom: parent.bottom
                        left: parent.left
                        right: parent.right
                    }

                    onJavaScriptConsoleMessage: (level, message, line, source) => {
                        console.log("# " + message + ": " + source + " +" + line)
                    }

                    onPermissionRequested: (permission) => {
                        console.log("#### permission requested", permission.isValid, permission.origin, permission.permissionType, permission.state)
                        permission.grant()
                    }

                    onLoadingChanged: (info) => {
                        if (info.status === WebEngineView.LoadSucceededStatus) {
                            jitsiView.runJavaScript(confConn.jitsiJavascriptInternal())
                        } else {
                            console.error(`Failed to load HTML: ${info.errorString} (domain: ${info.errorDomain}, code: ${info.errorCode}, status: ${info.status}, url: ${info.url})`)
                        }
                    }

                    Connections {
                        target: confConn
                        function onIsPasswordRequiredChanged() {
                            if (confConn.isPasswordRequired) {
                                ViewHelper.topDrawer.loader.sourceComponent = passwordItemComponent
                            } else {
                                ViewHelper.topDrawer.loader.sourceComponent = undefined
                            }
                        }
                    }
                }

                Component {
                    id: passwordItemComponent

                    Item {
                        id: passwordItem
                        implicitWidth: 360
                        implicitHeight: passwordItemColumn.implicitHeight

                        function respondPassword() {
                            confConn.enterPassword(passwordField.text, rememberCheckBox.checked)
                        }

                        function cancel() {
                            confConn.leaveConference()
                            ViewHelper.topDrawer.loader.sourceComponent = undefined
                        }

                        Column {
                            id: passwordItemColumn
                            topPadding: 20
                            spacing: 20
                            anchors {
                                left: parent.left
                                right: parent.right
                                verticalCenter: parent.verticalCenter
                            }

                            Label {
                                text: qsTr("This conference is protected by a password. Please enter it to join the room.")
                                wrapMode: Text.Wrap
                                anchors {
                                    left: parent.left
                                    right: parent.right
                                }
                            }

                            TextField {
                                id: passwordField
                                echoMode: TextInput.Password
                                placeholderText: qsTr("Password")
                                anchors {
                                    left: parent.left
                                    right: parent.right
                                }

                                Keys.onEnterPressed: () => passwordItem.respondPassword()
                                Keys.onReturnPressed: () => passwordItem.respondPassword()
                                Keys.onEscapePressed: () => passwordItem.cancel()
                                Component.onCompleted: () => passwordField.forceActiveFocus()
                            }

                            CheckBox {
                                id: rememberCheckBox
                                text: qsTr("Remember password")
                                anchors {
                                    left: parent.left
                                    right: parent.right
                                }
                            }

                            Row {
                                anchors.horizontalCenter: parent.horizontalCenter
                                spacing: 20

                                Button {
                                    text: qsTr("Cancel")

                                    onClicked: () => passwordItem.cancel()
                                }

                                Button {
                                    id: joinRoomButton
                                    text: qsTr("Join Room")
                                    enabled: passwordField.text.length > 0
                                    highlighted: true

                                    onClicked: () => passwordItem.respondPassword()
                                }
                            }
                        }
                    }
                }

                Component {
                    id: setPasswordItemComponent

                    Item {
                        id: setPasswordItem
                        implicitWidth: 360
                        implicitHeight: setPasswordItemColumn.implicitHeight

                        function setPassword(password : string) {
                            confConn.setRoomPassword(password)
                            ViewHelper.topDrawer.loader.sourceComponent = undefined
                        }

                        function cancel() {
                            ViewHelper.topDrawer.loader.sourceComponent = undefined
                        }

                        Column {
                            id: setPasswordItemColumn
                            topPadding: 20
                            spacing: 20
                            anchors {
                                left: parent.left
                                right: parent.right
                                verticalCenter: parent.verticalCenter
                            }

                            states: [
                                State {
                                    name: "PASSWORD_SET_BY_ME"
                                    when: confConn.isPasswordRequired && confConn.roomPassword !== ""

                                    PropertyChanges {
                                        passwordField.visible: false
                                        passwordLabel.visible: true
                                        copyClipboardButton.text: confConn.roomPassword
                                        newPasswordLabel.visible: false
                                        existingPasswordLabel.visible: true
                                        removePasswordButton.visible: true
                                        savePasswordButton.visible: false
                                    }
                                },
                                State {
                                    name: "PASSWORD_SET_BY_SOMEONE_ELSE"
                                    when: confConn.isPasswordRequired && confConn.roomPassword === ""

                                    PropertyChanges {
                                        passwordField.visible: false
                                        otherSetPasswordLabel.visible: true
                                        copyClipboardButton.visible: false
                                        newPasswordLabel.visible: false
                                        showPasswordSwitch.visible: false
                                        savePasswordButton.visible: false
                                    }
                                }
                            ]

                            Label {
                                id: newPasswordLabel
                                text: qsTr("Enter a password to protect this conference room. Other participants must enter it before taking part in the session.")
                                wrapMode: Text.Wrap
                                anchors {
                                    left: parent.left
                                    right: parent.right
                                }
                            }

                            Label {
                                id: existingPasswordLabel
                                visible: false
                                text: qsTr("This password has been set for the conference room and must be entered by participants before taking part in the session.")
                                wrapMode: Text.Wrap
                                anchors {
                                    left: parent.left
                                    right: parent.right
                                }
                            }

                            Label {
                                id: otherSetPasswordLabel
                                visible: false
                                text: qsTr("The room password has been set by someone else.")
                                wrapMode: Text.Wrap
                                anchors {
                                    left: parent.left
                                    right: parent.right
                                }
                            }

                            Item {
                                height: passwordField.implicitHeight
                                anchors {
                                    left: parent.left
                                    right: parent.right
                                }

                                TextField {
                                    id: passwordField
                                    echoMode: showPasswordSwitch.checked ? TextInput.Normal : TextInput.Password
                                    placeholderText: qsTr("Password")
                                    anchors {
                                        left: parent.left
                                        right: copyClipboardButton.left
                                        rightMargin: 10
                                    }

                                    Component.onCompleted: () => passwordField.forceActiveFocus()
                                }

                                Label {
                                    id: passwordLabel
                                    text: showPasswordSwitch.checked ? confConn.roomPassword : "*".repeat(confConn.roomPassword.length)
                                    visible: false
                                    font {
                                        family: "Mono"
                                        weight: Font.Medium
                                    }
                                    anchors {
                                        left: parent.left
                                        right: copyClipboardButton.left
                                        rightMargin: 10
                                        verticalCenter: parent.verticalCenter
                                    }
                                }

                                ClipboardButton {
                                    id: copyClipboardButton
                                    text: passwordField.text
                                    anchors {
                                        verticalCenter: parent.verticalCenter
                                        right: parent.right
                                    }
                                }
                            }

                            Switch {
                                id: showPasswordSwitch
                                text: qsTr("Show password")
                                anchors {
                                    left: parent.left
                                    right: parent.right
                                }
                            }

                            Row {
                                anchors.horizontalCenter: parent.horizontalCenter
                                spacing: 20

                                Button {
                                    text: qsTr("Cancel")

                                    onClicked: () => setPasswordItem.cancel()
                                }

                                Button {
                                    id: removePasswordButton
                                    visible: false
                                    text: qsTr("Remove")
                                    onClicked: () => setPasswordItem.setPassword("")
                                }

                                Button {
                                    id: savePasswordButton
                                    text: qsTr("Save")
                                    enabled: passwordField.text.length > 0
                                    highlighted: true

                                    onClicked: () => setPasswordItem.setPassword(passwordField.text)
                                }
                            }
                        }
                    }
                }

                Component {
                    id: videoQualityComponent

                    Item {
                        implicitWidth: 360
                        implicitHeight: videoQualityColumn.implicitHeight

                        Column {
                            id: videoQualityColumn
                            topPadding: 20
                            spacing: 5
                            anchors {
                                left: parent.left
                                right: parent.right
                                verticalCenter: parent.verticalCenter
                            }

                            component QualityButton : RadioButton {
                                id: qButton
                                checked: confConn.videoQuality === qButton.qualityValue
                                anchors {
                                    left: parent?.left
                                    right: parent?.right
                                }

                                required property int qualityValue

                                onToggled: () => confConn.setVideoQuality(qButton.qualityValue)
                            }

                            QualityButton {
                                text: qsTr("No video (audio only)")
                                qualityValue: IConferenceConnector.VideoQuality.AudioOnly
                            }

                            QualityButton {
                                text: qsTr("Lowest quality")
                                qualityValue: IConferenceConnector.VideoQuality.Low
                            }

                            QualityButton {
                                text: qsTr("Standard quality")
                                qualityValue: IConferenceConnector.VideoQuality.Average
                            }

                            QualityButton {
                                text: qsTr("Highest quality")
                                qualityValue: IConferenceConnector.VideoQuality.Maximum
                            }

                            Button {
                                text: qsTr("Close")
                                highlighted: true
                                anchors.right: parent.right

                                onClicked: () => ViewHelper.topDrawer.loader.sourceComponent = undefined
                            }
                        }
                    }
                }

                Component.onCompleted: {
                    jitsiView.loadHtml(confConn.jitsiHtmlInternal(), "https://" + GlobalInfo.jitsiUrl())
                }
            }
        }
    }

    Item {
        id: verticalDragbarDummy
        width: 24
        visible: callListCard.visible
        anchors {
            top: parent.top
            bottom: parent.bottom
            right: callListCard.left
        }

        HoverHandler {
            id: verticalDragbarDummyHoverHandler
            enabled: false
            cursorShape: Qt.SplitHCursor
        }

        DragHandler {
            id: verticalDragbarDummyDragHandler
            enabled: false
            yAxis.enabled: false
            xAxis {
                minimum: 1/2 * control.width
                maximum: control.width - callSideBar.optimalExtendedWidth - verticalDragbarDummy.width - callListCard.anchors.rightMargin
            }

            onActiveChanged: () => {
                // Make sure all widths update correctly again as dragging breaks the x binding
                if (!verticalDragbarDummyDragHandler.active) {
                    const factor = verticalDragbarDummy.x / control.width
                    verticalDragbarDummy.x = Qt.binding(() => factor * control.width)
                }
            }
        }
    }

    Card {
        id: callListCard
        width: 70
        visible: confConn.isInConference
        anchors {
            top: parent.top
            right: parent.right
            bottom: parent.bottom

            rightMargin: 24
            bottomMargin: 15
        }

        CallSideBar {
            id: callSideBar
            anchors.fill: parent
            chatAvailable: confConn.hasCapability(IConferenceConnector.Capability.ChatInCall)
            personsAvailable: confConn.hasCapability(IConferenceConnector.Capability.ParticipantRoles)
            iConferenceConnector: confConn

            Connections {
                target: confConn
                function onIsInConferenceChanged() {
                    if (!confConn.isInConference) {
                        callSideBar.selectedSideBarMode = CallSideBar.None
                    }
                }
            }

            Connections {
                target: ViewHelper
                function onShowConferenceChat() {
                    if (callSideBar.chatAvailable) {
                        callSideBar.selectedSideBarMode = CallSideBar.Chat
                    }
                }
            }
        }
    }
}
