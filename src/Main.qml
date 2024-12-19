pragma ComponentBehavior: Bound

import QtQuick
import QtCore
import QtQml.Models
import QtQuick.Controls.Material
import base

Item {
    id: baseItem

    Settings {
        id: settings
        location: ViewHelper.userConfigPath
        category: "generic"

        property bool showCallWindowOnStartup: false
        property bool showTrayDialog: true
        property alias dialWindowWidth: dialWindow.width
        property alias dialWindowHeight: dialWindow.height
    }

    Component.onCompleted: () => {
        DialogFactory.rootItem = baseItem

        if (settings.showCallWindowOnStartup) {
            dialWindow.show()
        }

        if (!ViewHelper.isSystrayAvailable() && settings.showTrayDialog) {
            const item = DialogFactory.createInfoDialog({
                                               title: qsTr("No system tray available"),
                                               text: qsTr("GOnnect provides quick access to functionality by providing a system tray. Your desktop environment does not provide one.")
                                           })
            item.accepted.connect(() => settings.showTrayDialog = false)
        }
    }

    Connections {
        id: viewHelperConnections
        target: ViewHelper

        property EmergencyCallIncomingWindow emergencyWindow: null

        function onActivateSearch() {
            dialWindow.show()
            dialWindow.raise()
            dialWindow.requestActivate()
            dialWindow.activateSearch()
        }
        function onShowSettingsWindow() {
            settingsWindow.show()
        }
        function onShowAboutWindow() {
            const item = aboutWindowComponent.createObject(baseItem)
            item.show()
        }
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
    }

    Connections {
        target: SIPAccountManager
        function onAuthorizationFailed(accountId : string) {
            DialogFactory.createDialog("AccountCredentialsDialog.qml", { accountId })
        }
    }

    DialWindow {
        id: dialWindow
        objectName: "dialWindow"
        visible: false

        onShowHistory: () => historyWindow.show()
        onShowSettings: () => settingsWindow.show()
        onShowCalls: () => callsWindow.show()
        onShowShortcuts: () => {
                             const item = shortcutsWindowComponent.createObject(baseItem)
                             item.show()
                         }
        onShowAbout: () => {
            const item = aboutWindowComponent.createObject(baseItem)
            item.show()
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

    BaseWindow {
        id: settingsWindow
        objectName: "settingsWindow"
        width: 610
        height: 910
        visible: true
        title: qsTr("Settings")

        minimumWidth: settingsWindow.width
        minimumHeight: settingsWindow.height
        maximumWidth: settingsWindow.width
        maximumHeight: settingsWindow.height

        readonly property Connections viewHelperConnections: Connections {
            target: ViewHelper
            function onShowAudioSettings() {
                settingsWindow.show()
                settingsPage.scrollToAudio()
            }
        }

        SettingsPage {
            id: settingsPage
            anchors.fill: parent
        }
    }

    Connections {
        target: SIPCallManager

        function onHasEstablishedCallsChanged() {
            if (SIPCallManager.hasEstablishedCalls) {
                if (callsWindow.visibility === Window.Hidden) {
                    callsWindow.show()
                }
            }
        }

        function onEarlyMediaActiveChanged() {
            if (SIPCallManager.earlyMediaActive) {
                if (callsWindow.visibility === Window.Hidden) {
                    callsWindow.show()
                }
            }
        }

        function onMeetingRequested(accountId, callId) {
            const item = DialogFactory.createDialog("JitsiUpgradeDialog.qml")
            item.accepted.connect((continueCall) => {
                SIPCallManager.triggerCapability(accountId, callId, continueCall ? "jitsi:openMeeting" : "jitsi:openMeeting:hangup")
            })
        }

        function onShowCallWindow() {
            if (callsWindow.visibility === Window.Hidden) {
                callsWindow.show()
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

    BaseWindow {
        id: callsWindow
        objectName: "callsWindow"
        width: 710
        height: 410
        visible: true
        title: qsTr("Current Calls")

        minimumWidth: callsWindow.width
        minimumHeight: callsWindow.height
        maximumWidth: callsWindow.width
        maximumHeight: callsWindow.height

        onClosing: closeEvent => {
            closeEvent.accepted = false

            if (SIPCallManager.hasActiveCalls) {
                const item = DialogFactory.createConfirmDialog({
                                                                   title: qsTr("End all calls"),
                                                                   text: qsTr("Do you really want to close this window and terminate all ongoing calls?")
                                                               })
                item.accepted.connect(() => {
                                          SIPCallManager.endAllCalls()
                                          callsWindow.hide()
                                      })
            } else {
                callsWindow.hide()
            }
        }

        CallsWindow {
            id: callsWindowItem
            anchors.fill: parent

            onCountChanged: () => {
                                if (!callsWindowItem.count) {
                                    callsWindow.hide()
                                }
                            }
        }
    }

    CompleteHistoryWindow {
        id: historyWindow
    }


    AudioEnvWindow {
        id: configureUnknownAudioEnvWindow

        readonly property Connections audioManagerConnections: Connections {
            target: SIPAudioManager
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
