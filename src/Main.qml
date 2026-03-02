pragma ComponentBehavior: Bound

import QtQuick
import QtCore
import base

Item {
    id: baseItem

    Settings {
        id: settings
        location: ViewHelper.userConfigPath
        category: "generic"

        property bool showMainWindowOnStart: true
        property bool showTrayDialog: true
    }

    Component.onCompleted: () => {
        DialogFactory.rootItem = baseItem

        if (settings.showMainWindowOnStart) {
            gonnectWindow.show()
        }

        if (!ViewHelper.isSystrayAvailable() && settings.showTrayDialog) {
            const item = DialogFactory.createInfoDialog({
                title: qsTr("No system tray available"),
                                                        text: qsTr("GOnnect provides quick access to functionality by providing a system tray. Your desktop environment does not provide one.")
            })
            item.accepted.connect(() => settings.showTrayDialog = false)
        }

        if (ViewHelper.unsupportedPlatform) {
            DialogFactory.createInfoDialog({
                        title: qsTr("Information"),
                        text: qsTr("GOnnect is under testing for your operating system and is not yet officially released. There is no feature parity with the Linux version yet, and there may be bugs that we didn't find yet. You're welcome with reporting these issues on github. Happy testing!")
                    })
        }
    }

    Connections {
        id: viewHelperConnections
        target: ViewHelper

        property EmergencyCallIncomingWindow emergencyWindow: null
        property var passwordDialogs: ({})

        function onActivateSearch() {
            gonnectWindow.ensureVisible()
            gonnectWindow.focusSearchBox()
        }
        function onShowSettings() {
            gonnectWindow.updateTabSelection(gonnectWindow.settingsPageId,
                                             GonnectWindow.PageType.Settings)
            gonnectWindow.ensureVisible()
        }
        function onShowShortcuts() { shortcutsWindowComponent.createObject(baseItem).show() }
        function onShowAbout() { aboutWindowComponent.createObject(baseItem).show() }
        function onShowQuitConfirm() {
            const item = DialogFactory.createConfirmDialog({ text: qsTr("There are still phone calls going on, do you really want to quit?") })
            item.accepted.connect(() => ViewHelper.quitApplicationNoConfirm())
        }
        function onShowEmergency(accountId : string, callId : int, displayName : string) {
            const item = emergencyWindowComponent.createObject(baseItem, { accountId, callId, displayName })
            item.show()
            item.raise()
            viewHelperConnections.emergencyWindow = item
        }
        function onHideEmergency() {
            if (viewHelperConnections.emergencyWindow) {
                viewHelperConnections.emergencyWindow.hide()
                viewHelperConnections.emergencyWindow = null
            }
        }

        function onOpenMeetingRequested(meetingId : string, displayName : string, startFlags : int, callHistoryItem : variant) {
            gonnectWindow.ensureVisible()

            Qt.callLater(() => {
                gonnectWindow.openMeeting(meetingId, displayName, startFlags, callHistoryItem)
            })
        }

        function onPasswordRequested(id : string, host : string) {
            const existingDialogs = viewHelperConnections.passwordDialogs

            if (!existingDialogs[id]) {
                const dialog = DialogFactory.createDialog("CredentialsDialog.qml", { text: qsTr("Please enter the password for %1:").arg(host) })
                dialog.onPasswordAccepted.connect(pw => ViewHelper.respondPassword(id, pw))

                viewHelperConnections.passwordDialogs[id] = dialog
                dialog.Component.destruction.connect(() => delete viewHelperConnections.passwordDialogs[id])
            } else {
                existingDialogs[id].show()
            }
        }

        function onRecoveryKeyRequested(id : string, displayName : string) {
            const dialog = DialogFactory.createDialog("CredentialsDialog.qml", { text: qsTr("Please enter the recovery key for %1:").arg(displayName) })
            dialog.onPasswordAccepted.connect(key => ViewHelper.respondRecoveryKey(id, key))
        }
    }

    GonnectWindow {
        id: gonnectWindow

        onClosing: closeEvent => {
            closeEvent.accepted = false

            if (GlobalCallState.globalCallState & (ICallState.State.CallActive
                | ICallState.State.RingingIncoming
                | ICallState.State.RingingOutgoing)) {
                const item = DialogFactory.createConfirmDialog({
                    title: qsTr("End all calls"),
                                                               text: qsTr("Do you really want to close this window and terminate all ongoing calls?")
                })
                item.accepted.connect(() => {
                    SIPCallManager.endAllCalls()
                    gonnectWindow.hide()
                })
                } else {
                    gonnectWindow.hide()
                }
        }
    }

    Connections {
        target: SIPAccountManager
        function onAuthorizationFailed(accountId : string) {
            const dialog = DialogFactory.createDialog("CredentialsDialog.qml", { text: qsTr("Please enter the password for the SIP account:") })
            dialog.onPasswordAccepted.connect(pw => SIPAccountManager.setAccountCredentials(accountId, pw))
        }
    }

    Component {
        id: shortcutsWindowComponent
        ShortcutsWindow {}
    }

    Component {
        id: aboutWindowComponent
        AboutWindow {}
    }

    Component {
        id: emergencyWindowComponent
        EmergencyCallIncomingWindow {}
    }

    Component {
        id: sipTemplateWizardComponent
        SipTemplateWizard {}
    }

    Connections {
        target: GlobalCallState

        function onGlobalCallStateChanged() {
            if (GlobalCallState.globalCallState & (ICallState.State.CallActive | ICallState.State.RingingIncoming)) {
                gonnectWindow.ensureVisible()
            }
        }
    }

    Connections {
        target: SIPManager

        function onNotConfigured() {
            const item = sipTemplateWizardComponent.createObject(baseItem)
            item.show()
        }
    }

    AudioEnvWindow {
        id: configureUnknownAudioEnvWindow

        readonly property Connections audioManagerConnections: Connections {
            target: AudioManager
            function onNoMatchingAudioProfile() {
                configureUnknownAudioEnvWindow.show()
            }

            function onMatchingAudioProfile() {
                configureUnknownAudioEnvWindow.close()
            }
        }
    }

    Connections {
        target: ErrorBus
        function onError(msg : string) {
            DialogFactory.createInfoDialog({
                title: qsTr("Error"),
                text: msg
            })
        }
        function onFatalError(msg : string) {
            const item = DialogFactory.createInfoDialog({
                title: qsTr("Fatal Error"),
                text: msg
            })
            item.accepted.connect(() => Qt.quit())
        }
    }

}
