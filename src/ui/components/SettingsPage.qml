pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls.Material
import QtQuick.Controls.Material.impl
import QtQuick.Controls.impl
import base

Item {
    id: control

    function scrollToAudio() {
        // Use timer because the items (especially list elements from repeater) must be rendered first
        scrollToAudioTimer.start()
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.backgroundSecondaryColor

        Accessible.ignored: true
    }

    Timer {
        id: scrollToAudioTimer
        interval: 10
        repeat: false
        onTriggered: () => flickable.contentY = audioSettingsCard.y - 20
    }

    Settings {
        id: genericSettings
        location: ViewHelper.userConfigPath
        category: "generic"

        property alias showMainWindowOnStart: startInBackgroundCheckBox.checked
        property alias busyOnBusy: busyOnBusyCheckBox.checked
        property alias inverseAcceptReject: inverseAcceptRejectCheckBox.checked
        property alias headsetHookOff: headsetHookOffCheckBox.checked
        property alias disableMutePropagation: disableMutePropagationCheckBox.checked
        property alias useHeadset: headsetCheckBox.checked
        property alias noSyncSystemMute: disableSystemMutePropagationCheckBox.checked
        property alias jitsiChatAsNotifications: jitsiChatAsNotificationsCheckBox.checked
        property alias keepHistoryDays: historyDaysToKeepInputField.text
    }

    Settings {
        id: audioSettings
        location: ViewHelper.userConfigPath
        category: "audio" + AudioManager.currentProfile

        property string notificationTone
        property alias notificationVolume: notificationToneVolumeSlider.value

        property string ringtone
        property alias ringtonePause: ringTonePauseSlider.value
        property alias ringtoneVolume: ringToneVolumeSlider.value
        property alias preferExternalRinger: externalRingerCheckbox.checked
    }

    Flickable {
        id: flickable
        anchors.fill: parent
        contentHeight: flickableContainer.implicitHeight
        bottomMargin: 20
        clip: true

        ScrollBar.vertical: ScrollBar { width: 10 }

        readonly property int columns: Math.max(1, Math.min(Math.floor(flickableContainer.width / 600), 3))
        readonly property int columnWidth: Math.floor(flickableContainer.width / flickable.columns)

        states: [
            State {
                name: "THREE_COLUMN_STATE"
                extend: "TWO_COLUMN_STATE"
                when: flickable.columns === 3

                PropertyChanges {
                    thirdColumn.width: flickable.columnWidth
                    thirdColumn.anchors.topMargin: 0
                }
                AnchorChanges {
                    target: thirdColumn
                    anchors {
                        top: flickableContainer.top
                        right: undefined
                        left: secondColumn.right
                    }
                }
            },
            State {
                name: "TWO_COLUMN_STATE"
                when: flickable.columns === 2

                PropertyChanges {
                    firstColumn.width: flickable.columnWidth
                    secondColumn.width: flickable.columnWidth
                    secondColumn.anchors.topMargin: 0
                }
                AnchorChanges {
                    target: firstColumn
                    anchors.right: undefined
                }
                AnchorChanges {
                    target: secondColumn
                    anchors {
                        top: flickableContainer.top
                        right: undefined
                        left: firstColumn.right
                    }
                }
            }
        ]

        Item {
            id: flickableContainer
            implicitHeight: Math.max(firstColumn.y  + firstColumn.implicitHeight,
                                     secondColumn.y + secondColumn.implicitHeight,
                                     thirdColumn.y  + thirdColumn.implicitHeight)
            anchors {
                left: parent.left
                right: parent.right
                leftMargin: 20
                rightMargin: 20
            }

            Column {
                id: firstColumn
                spacing: 30
                anchors {
                    left: parent.left
                    right: parent.right
                }

                CardList {
                    id: settingsCard
                    title: qsTr('Settings')
                    spacing: 20
                    anchors {
                        left: parent.left
                        right: parent.right
                        margins: 20
                    }

                    Column {
                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                        CheckBox {
                            id: startInBackgroundCheckBox
                            text: qsTr('Show main window on startup')
                            checked: true  // default value
                            anchors {
                                left: parent.left
                                right: parent.right
                            }

                            Accessible.role: Accessible.CheckBox
                            Accessible.name: startInBackgroundCheckBox.text
                            Accessible.focusable: true
                        }

                        CheckBox {
                            id: inverseAcceptRejectCheckBox
                            text: qsTr('Inverse Accept / Reject buttons')
                            anchors {
                                left: parent.left
                                right: parent.right
                            }

                            Accessible.role: Accessible.CheckBox
                            Accessible.name: inverseAcceptRejectCheckBox.text
                            Accessible.focusable: true
                        }

                        CheckBox {
                            id: jitsiChatAsNotificationsCheckBox
                            text: qsTr('Show chat messages as desktop notifications')
                            checked: true  // default value
                            anchors {
                                left: parent.left
                                right: parent.right
                            }

                            Accessible.role: Accessible.CheckBox
                            Accessible.name: jitsiChatAsNotificationsCheckBox.text
                            Accessible.focusable: true
                        }

                        CheckBox {
                            id: headsetCheckBox

                            readonly property HeadsetDeviceProxy headsetProxy: ViewHelper.headsetDeviceProxy()

                            text: qsTr('Enable USB headset driver [%1]').arg(headsetProxy.name === "" ? qsTr("not detected") : headsetProxy.name)
                            anchors {
                                left: parent.left
                                right: parent.right
                            }

                            Accessible.role: Accessible.CheckBox
                            Accessible.name: headsetCheckBox.text
                            Accessible.focusable: true
                        }

                        CheckBox {
                            id: disableMutePropagationCheckBox
                            text: qsTr('Disable USB headset mute state propagation')
                            enabled: headsetCheckBox.checked
                            anchors {
                                left: parent.left
                                right: parent.right
                            }

                            Accessible.role: Accessible.CheckBox
                            Accessible.name: disableMutePropagationCheckBox.text
                            Accessible.focusable: true
                        }

                        CheckBox {
                            id: disableSystemMutePropagationCheckBox
                            text: qsTr('Disable synchronisation with the system mute state')  + " (" + qsTr("restart required") + ")"
                            enabled: ViewHelper.canSyncSystemMute
                            anchors {
                                left: parent.left
                                right: parent.right
                            }

                            Accessible.role: Accessible.CheckBox
                            Accessible.name: disableSystemMutePropagationCheckBox.text
                            Accessible.focusable: true
                        }

                        CheckBox {
                            id: headsetHookOffCheckBox
                            text: qsTr('Show dial window on USB headset pick up')
                            enabled: headsetCheckBox.checked
                            anchors {
                                left: parent.left
                                right: parent.right
                            }

                            Accessible.role: Accessible.CheckBox
                            Accessible.name: headsetHookOffCheckBox.text
                            Accessible.focusable: true
                        }
                    }
                }

                CardList {
                    title: qsTr('History')
                    spacing: 20
                    anchors {
                        left: parent.left
                        right: parent.right
                        margins: 20
                    }

                    Item {
                        implicitHeight: historyDaysToKeepInputField.implicitHeight
                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                        TextField {
                            id: historyDaysToKeepInputField
                            text: "90"
                            anchors.left: parent.left
                            topPadding: 0
                            bottomPadding: 0
                            inputMethodHints: Qt.ImhDigitsOnly
                            validator: IntValidator {
                                bottom: 1
                                top: 999
                            }
                        }

                        Label {
                            text: qsTr("day(s) of history", "", parseInt(historyDaysToKeepInputField.text, 10))
                            anchors {
                                left: historyDaysToKeepInputField.right
                                right: parent.right
                                verticalCenter: historyDaysToKeepInputField.verticalCenter
                                leftMargin: Theme.d
                            }
                        }
                    }

                    Label {
                        id: historyExplanationLabel
                        color: Theme.secondaryTextColor
                        wrapMode: Label.Wrap
                        anchors {
                            left: parent.left
                            right: parent.right
                        }
                        text: qsTr("Keep a call history for this number of days (from 1 to 999). Any entry before this time span is automatically removed. Changing this setting has an effect on the next day or a restart of GOnnect.")
                    }
                }

                CardList {
                    title: qsTr('Appearance')
                    anchors {
                        left: parent.left
                        right: parent.right
                        margins: 20
                    }

                    CheckBox {
                        id: windowDeocorationCheckbox
                        text: qsTr("Use custom window decoration") + " (" + qsTr("restart required") + ")"
                        checked: Theme.useOwnDecoration
                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                        onToggled: () => Theme.setUseOwnDecoration(windowDeocorationCheckbox.checked)

                        Accessible.role: Accessible.CheckBox
                        Accessible.name: windowDeocorationCheckbox.text
                        Accessible.focusable: true
                        Accessible.onToggleAction: () => Theme.setUseOwnDecoration(windowDeocorationCheckbox.checked)

                        Connections {
                            target: Theme
                            function onUseOwnDecorationChanged() {
                                windowDeocorationCheckbox.checked = Theme.useOwnDecoration
                            }
                        }
                    }

                    CheckBox {
                        id: trayIconDark
                        text: qsTr('Use dark mode tray icon')
                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                        property bool initialized: false

                        Component.onCompleted: () => {
                            let val = genericSettings.value('trayIconDark', false)
                            if (typeof val === "string") {  // Value read from settings seems to be a string sometimes...
                                val = val === "true"
                            }

                            trayIconDark.checked = val
                            trayIconDark.initialized = true
                        }

                        onToggled: () => trayIconDark.setTrayIconTheme()

                        Accessible.role: Accessible.CheckBox
                        Accessible.name: trayIconDark.text
                        Accessible.focusable: true
                        Accessible.onToggleAction: () => trayIconDark.setTrayIconTheme()

                        function setTrayIconTheme() {
                            if (trayIconDark.initialized) {
                                genericSettings.setValue('trayIconDark', trayIconDark.checked)
                                ViewHelper.resetTrayIcon()
                            }
                        }
                    }

                    LabeledItem {
                        topPadding: 20
                        text: qsTr('Color scheme')
                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                        ComboBox {
                            id: darkModeComboBox
                            editable: false
                            textRole: 'displayName'
                            valueRole: 'value'
                            rightPadding: 10
                            anchors {
                                left: parent.left
                                right: parent.right
                            }
                            model: ListModel {
                                id: themeEntries
                                ListElement {
                                    displayName: qsTr("System default")
                                    value: Theme.ThemeVariant.System
                                }
                                ListElement {
                                    displayName: qsTr("Light")
                                    value: Theme.ThemeVariant.Light
                                }
                                ListElement {
                                    displayName: qsTr("Dark")
                                    value: Theme.ThemeVariant.Dark
                                }
                            }

                            Accessible.role: Accessible.ComboBox
                            Accessible.name: qsTr("Theme selection box")
                            Accessible.description: qsTr("Select the UI theme")

                            delegate: ItemDelegate {
                                id: darkModeDelg
                                text: darkModeDelg.displayName
                                width: parent?.width ?? 0

                                font.family: darkModeComboBox.font.family
                                font.weight: darkModeComboBox.font.weight
                                font.pointSize: darkModeComboBox.font.pointSize

                                Accessible.role: Accessible.ListItem
                                Accessible.name: darkModeDelg.displayName
                                Accessible.description: qsTr("Currently selected theme option")
                                Accessible.focusable: true

                                required property string displayName
                            }

                            onActivated: {
                                Theme.themeVariant = darkModeComboBox.currentValue
                            }

                            Component.onCompleted: () => darkModeComboBox.setDefaultValue()

                            Connections {
                                target: Theme
                                function onThemeVariantChanged() { darkModeComboBox.setDefaultValue() }
                            }

                            function setDefaultValue() {
                                darkModeComboBox.currentIndex = Theme.themeVariant
                            }
                        }
                    }
                }

                CardList {
                    id: phoningCard
                    title: qsTr('Phoning')
                    anchors {
                        left: parent?.left
                        right: parent?.right
                        margins: 20
                    }

                    CheckBox {
                        id: busyOnBusyCheckBox
                        text: qsTr('Signalling busy when a call is active')
                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                        Accessible.role: Accessible.CheckBox
                        Accessible.name: busyOnBusyCheckBox.text
                        Accessible.focusable: true
                    }

                    Repeater {
                        id: togglerRepeater
                        model: TogglerProxyModel {
                            displayFilter: Toggler.CFG_PHONING
                            TogglerModel {}
                        }

                        delegate: CheckBox {
                            id: togglerDelegate
                            text: togglerDelegate.name
                            checked: togglerDelegate.isActive
                            enabled: !togglerDelegate.isBusy
                            indicator: togglerDelegate.isBusy ? busyIndicator : idleIndicator
                            anchors {
                                left: parent?.left
                                right: parent?.right
                            }

                            onToggled: () => TogglerManager.toggleToggler(togglerDelegate.id)

                            Accessible.role: Accessible.CheckBox
                            Accessible.name: togglerDelegate.text
                            Accessible.focusable: true
                            Accessible.onToggleAction: () => TogglerManager.toggleToggler(togglerDelegate.id)

                            required property string id
                            required property string name
                            required property bool isActive
                            required property bool isBusy

                            states: [
                                State {
                                    when: togglerDelegate.isBusy
                                    PropertyChanges {
                                        busyIndicator.visible: true
                                        idleIndicator.visible: false
                                    }
                                }
                            ]

                            CheckIndicator {
                                id: idleIndicator
                                x: togglerDelegate.text ? (togglerDelegate.mirrored ? togglerDelegate.width - width - togglerDelegate.rightPadding : togglerDelegate.leftPadding) : togglerDelegate.leftPadding + (togglerDelegate.availableWidth - width) / 2
                                y: togglerDelegate.topPadding + (togglerDelegate.availableHeight - height) / 2
                                control: togglerDelegate

                                Ripple {
                                    x: (parent.width - width) / 2
                                    y: (parent.height - height) / 2
                                    width: 28; height: 28

                                    z: -1
                                    anchor: togglerDelegate
                                    pressed: togglerDelegate.pressed
                                    active: enabled && (togglerDelegate.down || togglerDelegate.visualFocus || togglerDelegate.hovered)
                                    color: togglerDelegate.checked ? togglerDelegate.Material.highlightedRippleColor : togglerDelegate.Material.rippleColor
                                }
                            }

                            Item {
                                id: busyIndicator
                                visible: false
                                x: togglerDelegate.text ? (togglerDelegate.mirrored ? togglerDelegate.width - width - togglerDelegate.rightPadding : togglerDelegate.leftPadding) : togglerDelegate.leftPadding + (togglerDelegate.availableWidth - width) / 2
                                y: togglerDelegate.topPadding + (togglerDelegate.availableHeight - height) / 2
                                implicitWidth: 18
                                implicitHeight: 18

                                IconLabel {
                                    anchors.centerIn: parent
                                    icon {
                                        width: 16
                                        height: 16
                                        color: Theme.secondaryTextColor
                                        source: Icons.viewRefresh
                                    }
                                }

                                Accessible.ignored: true
                            }
                        }
                    }
                }

                CardList {
                    id: identityCard
                    title: qsTr('Rules for telephone number transmission')
                    spacing: 20
                    anchors {
                        left: parent?.left
                        right: parent?.right
                        margins: 20
                    }

                    LabeledItem {
                        text: qsTr('Standard preferred identity')
                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                        ComboBox {
                            id: standardPreferredIdentitySelector
                            editable: false
                            textRole: 'displayName'
                            valueRole: 'id'
                            rightPadding: 10
                            anchors {
                                left: parent.left
                                right: parent.right
                            }
                            model: [
                                {
                                    displayName: qsTr("Default"),
                                    id: "default"
                                }, {
                                    displayName: qsTr("Auto"),
                                    id: "auto"
                                }
                            ].concat(SIPManager.preferredIdentities.filter(pi => pi.enabled))

                            Accessible.role: Accessible.ComboBox
                            Accessible.name: qsTr("Prefererred identity selection")
                            Accessible.description: qsTr("Select the preferred identity")

                            delegate: ItemDelegate {
                                id: standardPreferredIdentityDelg
                                text: standardPreferredIdentityDelg.displayName
                                width: parent?.width ?? 0

                                font.family: standardPreferredIdentitySelector.font.family
                                font.weight: standardPreferredIdentitySelector.font.weight
                                font.pointSize: standardPreferredIdentitySelector.font.pointSize

                                Accessible.role: Accessible.ListItem
                                Accessible.name: standardPreferredIdentityDelg.displayName
                                Accessible.description: qsTr("Currently selected identity option")
                                Accessible.focusable: true

                                required property string displayName
                            }

                            function setDefaultIdentity() {
                                const selectedId = SIPManager.defaultPreferredIdentity
                                const model = standardPreferredIdentitySelector.model

                                for (let i = 0; i < model.length; ++i) {
                                    if (model[i].id === selectedId) {
                                        standardPreferredIdentitySelector.currentIndex = i
                                        return
                                    }
                                }

                                standardPreferredIdentitySelector.currentIndex = -1
                            }

                            Component.onCompleted: () => standardPreferredIdentitySelector.setDefaultIdentity()

                            Connections {
                                target: SIPManager
                                function onPreferredIdentitiesChanged() { standardPreferredIdentitySelector.setDefaultIdentity() }
                                function onDefaultPreferredIdentityChanged() { standardPreferredIdentitySelector.setDefaultIdentity() }
                            }

                            onActivated: () => SIPManager.defaultPreferredIdentity = standardPreferredIdentitySelector.currentValue
                        }
                    }

                    Rectangle {
                        color: Theme.backgroundOffsetColor
                        radius: 12
                        height: prefIdentityCol.implicitHeight
                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                        Column {
                            id: prefIdentityCol
                            topPadding: 10
                            bottomPadding: 10
                            anchors {
                                left: parent.left
                                right: parent.right
                                leftMargin: 20
                                rightMargin: 20
                            }

                            Label {
                                id: prefIdentityEmpty
                                anchors.horizontalCenter: parent.horizontalCenter
                                visible: preferredIdentitiesRepeater.count === 0
                                text: qsTr("No preferred identities yet.")

                                Accessible.role: Accessible.StaticText
                                Accessible.name: prefIdentityEmpty.text
                            }

                            Repeater {
                                id: preferredIdentitiesRepeater
                                model: SIPManager.preferredIdentities
                                delegate: Item {
                                    id: preferredIdentityDelegate
                                    height: 50
                                    anchors {
                                        left: parent?.left
                                        right: parent?.right
                                    }

                                    required property int index
                                    required property PreferredIdentity modelData

                                    Accessible.role: Accessible.StaticText
                                    Accessible.name: preferredIdentityLabel.text
                                    Accessible.description: qsTr("Currently highlighted preferred identity. Tap to edit.")

                                    Rectangle {
                                        visible: delegateHoverHandler.hovered
                                        color: Theme.backgroundOffsetHoveredColor
                                        radius: 8
                                        anchors {
                                            fill: parent
                                            leftMargin: -10
                                            rightMargin: -10
                                            topMargin: 5
                                            bottomMargin: 5
                                        }

                                        Accessible.ignored: true
                                    }

                                    Rectangle {
                                        id: borderTop
                                        color: Theme.borderColor
                                        height: 1
                                        visible: preferredIdentityDelegate.index !== 0
                                        anchors {
                                            top: parent.top
                                            left: parent.left
                                            right: parent.right
                                        }

                                        Accessible.ignored: true
                                    }

                                    Label {
                                        id: preferredIdentityLabel
                                        enabled: preferredIdentityDelegate.modelData?.enabled ?? false
                                        text: preferredIdentityDelegate.modelData?.displayName
                                              + (preferredIdentityDelegate.modelData?.prefix
                                                 ? (" / "  + preferredIdentityDelegate.modelData?.prefix)
                                                 : "")
                                        elide: Text.ElideRight

                                        width: {
                                            let width = parent.width - 10
                                            if (standardBadge.visible) {
                                                width -= standardBadge.width
                                            }
                                            if (autoBadge.visible) {
                                                width -= autoBadge.width
                                            }
                                            return width
                                        }

                                        anchors {
                                            verticalCenter: parent.verticalCenter
                                            left: parent.left
                                        }

                                        Accessible.ignored: true
                                    }

                                    Badge {
                                        id: standardBadge
                                        text: qsTr("Standard")
                                        color: Theme.darkGreenColor
                                        visible: SIPManager.defaultPreferredIdentity === preferredIdentityDelegate.modelData?.id
                                        anchors {
                                            verticalCenter: parent.verticalCenter
                                            right: autoBadge.visible ? autoBadge.left : parent.right
                                            rightMargin: autoBadge.visible ? 10 : 0
                                        }
                                    }

                                    Badge {
                                        id: autoBadge
                                        text: qsTr("Auto")
                                        color: Theme.greenColor
                                        visible: preferredIdentityDelegate.modelData?.automatic ?? false
                                        anchors {
                                            verticalCenter: parent.verticalCenter
                                            right: parent.right
                                        }
                                    }

                                    HoverHandler {
                                        id: delegateHoverHandler
                                    }

                                    TapHandler {
                                        onTapped: () => preferredIdentityEditWindowComponent.createObject(
                                                      control,
                                                      { preferredIdentity: preferredIdentityDelegate.modelData })
                                    }
                                }
                            }
                        }
                    }

                    Button {
                        id: addIdentityButton
                        icon.source: Icons.listAdd
                        icon.width: 16
                        icon.height: 16
                        text: qsTr('Add identity')
                        anchors.right: parent.right

                        Material.elevation: 0

                        onClicked: () => {
                            const identity = SIPManager.addEmptyPreferredIdentity()
                            preferredIdentityEditWindowComponent.createObject(
                                control, {
                                    preferredIdentity: identity,
                                    isNew: true
                                })
                        }

                        Accessible.role: Accessible.Button
                        Accessible.name: addIdentityButton.text
                        Accessible.description: qsTr("Add a new preferred identity entry")
                        Accessible.focusable: true
                        Accessible.onPressAction: () => addIdentityButton.click()
                    }
                }
            }

            Column {
                id: secondColumn
                spacing: firstColumn.spacing
                leftPadding: 30
                anchors {
                    top: firstColumn.bottom
                    topMargin: secondColumn.spacing
                    left: parent.left
                    right: parent.right
                }

                CardList {
                    id: audioSettingsCard
                    title: qsTr('Audio settings')
                    spacing: 20
                    anchors {
                        left: parent?.left
                        right: parent?.right
                        margins: 20
                    }

                    CheckBox {
                        id: externalRingerCheckbox
                        text: qsTr('Prefer USB headset ring sound if available')
                        enabled: headsetCheckBox.checked
                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                        Accessible.role: Accessible.CheckBox
                        Accessible.name: externalRingerCheckbox.text
                        Accessible.focusable: true
                    }

                    LabeledItem {
                        text: qsTr('Input device')
                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                        ComboBox {
                            id: inputAudioSelector
                            editable: false
                            textRole: 'name'
                            valueRole: 'id'
                            rightPadding: 10
                            anchors {
                                left: parent.left
                                right: parent.right
                            }
                            model: AudioManager.devices.filter(device => device.isInput)

                            Accessible.role: Accessible.ComboBox
                            Accessible.name: qsTr("Input device selection")
                            Accessible.description: qsTr("Select the input device to be used")

                            delegate: ItemDelegate {
                                id: inputAudioSelectorDelg
                                text: inputAudioSelectorDelg.name
                                width: parent?.width ?? 0

                                font.family: inputAudioSelector.font.family
                                font.weight: inputAudioSelector.font.weight
                                font.pointSize: inputAudioSelector.font.pointSize

                                Accessible.role: Accessible.ListItem
                                Accessible.name: inputAudioSelectorDelg.name
                                Accessible.description: qsTr("Currently selected input option")
                                Accessible.focusable: true

                                required property string name
                            }

                            function updateSelectedAudioDeviceFromModel() {
                                const deviceId = AudioManager.captureDeviceId
                                const model = inputAudioSelector.model

                                for (let i = 0; i < model.length; ++i) {
                                    if (model[i].id === deviceId) {
                                        inputAudioSelector.currentIndex = i
                                        return
                                    }
                                }
                                inputAudioSelector.currentIndex = 0
                            }

                            onActivated: () => AudioManager.captureDeviceId = inputAudioSelector.currentValue

                            Component.onCompleted: () => inputAudioSelector.updateSelectedAudioDeviceFromModel()
                            onModelChanged: () => inputAudioSelector.updateSelectedAudioDeviceFromModel()

                            Connections {
                                target: AudioManager
                                function onCaptureDeviceIdChanged() { inputAudioSelector.updateSelectedAudioDeviceFromModel() }
                            }
                        }
                    }

                    LabeledItem {
                        text: qsTr('Output device')
                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                        ComboBox {
                            id: outputAudioSelector
                            editable: false
                            textRole: 'name'
                            valueRole: 'id'
                            rightPadding: 10
                            anchors {
                                left: parent.left
                                right: parent.right
                            }
                            model: AudioManager.devices.filter(device => device.isOutput)

                            Accessible.role: Accessible.ComboBox
                            Accessible.name: qsTr("Output device selection")
                            Accessible.description: qsTr("Select the output device to be used")

                            delegate: ItemDelegate {
                                id: outputAudioSelectorDelg
                                text: outputAudioSelectorDelg.name
                                width: parent?.width ?? 0

                                font.family: outputAudioSelector.font.family
                                font.weight: outputAudioSelector.font.weight
                                font.pointSize: outputAudioSelector.font.pointSize

                                Accessible.role: Accessible.ListItem
                                Accessible.name: outputAudioSelectorDelg.name
                                Accessible.description: qsTr("Currently selected output option")
                                Accessible.focusable: true

                                required property string name
                            }

                            function updateSelectedAudioDeviceFromModel() {
                                const deviceId = AudioManager.playbackDeviceId
                                const model = outputAudioSelector.model

                                for (let i = 0; i < model.length; ++i) {
                                    if (model[i].id === deviceId) {
                                        outputAudioSelector.currentIndex = i
                                        return
                                    }
                                }
                                outputAudioSelector.currentIndex = 0
                            }

                            onActivated: () => AudioManager.playbackDeviceId = outputAudioSelector.currentValue

                            Component.onCompleted: () => outputAudioSelector.updateSelectedAudioDeviceFromModel()
                            onModelChanged: () => outputAudioSelector.updateSelectedAudioDeviceFromModel()

                            Connections {
                                target: AudioManager
                                function onPlaybackDeviceIdChanged() { outputAudioSelector.updateSelectedAudioDeviceFromModel() }
                            }
                        }
                    }

                    LabeledItem {
                        text: qsTr('Output device for ring tone')
                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                        ComboBox {
                            id: outputRingToneAudioSelector
                            editable: false
                            textRole: 'name'
                            valueRole: 'id'
                            rightPadding: 10
                            anchors {
                                left: parent.left
                                right: parent.right
                            }
                            model: AudioManager.devices.filter(device => device.isOutput)

                            Accessible.role: Accessible.ComboBox
                            Accessible.name: qsTr("Prefererred identity selection")
                            Accessible.description: qsTr("Select the preferred identity")

                            delegate: ItemDelegate {
                                id: outputRingAudioSelectorDelg
                                text: outputRingAudioSelectorDelg.name
                                width: parent?.width ?? 0

                                font.family: outputRingToneAudioSelector.font.family
                                font.weight: outputRingToneAudioSelector.font.weight
                                font.pointSize: outputRingToneAudioSelector.font.pointSize

                                Accessible.role: Accessible.ListItem
                                Accessible.name: outputRingAudioSelectorDelg.name
                                Accessible.description: qsTr("Currently selected ring output option")
                                Accessible.focusable: true

                                required property string name
                            }

                            function updateSelectedAudioDeviceFromModel() {
                                const deviceId = AudioManager.ringDeviceId
                                const model = outputRingToneAudioSelector.model

                                for (let i = 0; i < model.length; ++i) {
                                    if (model[i].id === deviceId) {
                                        outputRingToneAudioSelector.currentIndex = i
                                        return
                                    }
                                }
                                outputRingToneAudioSelector.currentIndex = 0
                            }

                            onActivated: () => AudioManager.ringDeviceId = outputRingToneAudioSelector.currentValue ?? ""

                            Component.onCompleted: () => outputRingToneAudioSelector.updateSelectedAudioDeviceFromModel()
                            onModelChanged: () => outputRingToneAudioSelector.updateSelectedAudioDeviceFromModel()

                            Connections {
                                target: AudioManager
                                function onRingDeviceIdChanged() { outputRingToneAudioSelector.updateSelectedAudioDeviceFromModel() }
                            }
                        }
                    }

                    LabeledItem {
                        text: qsTr('Ring tone')
                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                        AudioFileSelector {
                            filePath: audioSettings.ringtone
                            isPlaying: ViewHelper.isPlayingRingTone
                            anchors {
                                left: parent.left
                                right: parent.right
                            }

                            onStartTestPlay: () => ViewHelper.testPlayRingTone(ringToneVolumeSlider.value / 100.0)
                            onStopTestPlay: () => ViewHelper.stopTestPlayRingTone()
                            onFileSelected: newFilePath => audioSettings.ringtone = newFilePath
                        }
                    }

                    LabeledItem {
                        text: qsTr('Ring tone volume')
                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                        VolumeSlider {
                            id: ringToneVolumeSlider
                            anchors {
                                left: parent.left
                                right: parent.right
                            }

                            onMoved: () => ViewHelper.testPlayRingTone(ringToneVolumeSlider.value / 100.0)
                        }
                    }

                    LabeledItem {
                        text: qsTr('Pause between ring tones [s]')
                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                        Item {
                            height: ringTonePauseSlider.height
                            anchors {
                                left: parent.left
                                right: parent.right
                            }

                            Slider {
                                id: ringTonePauseSlider
                                from: 0
                                to: 3000
                                stepSize: 250
                                value: 2500
                                anchors {
                                    left: parent.left
                                    right: ringTonePauseValueLabel.left
                                    rightMargin: 20
                                }

                                Accessible.role: Accessible.Slider
                                Accessible.focusable: true
                                Accessible.onIncreaseAction: () => {
                                    if (ringTonePauseSlider.value < ringTonePauseSlider.to) {
                                        ringTonePauseSlider.value += ringTonePauseSlider.stepSize
                                    }
                                }
                                Accessible.onDecreaseAction: () => {
                                    if (ringTonePauseSlider.value > ringTonePauseSlider.from) {
                                        ringTonePauseSlider.value -= ringTonePauseSlider.stepSize
                                    }
                                }
                            }

                            Label {
                                id: ringTonePauseValueLabel
                                //: Label for showing seconds
                                text: qsTr("%1 s").arg((ringTonePauseSlider.value / 1000).toLocaleString(Qt.locale(), "f", 2))
                                horizontalAlignment: Label.AlignRight
                                width: 40
                                anchors {
                                    right: parent.right
                                    verticalCenter: ringTonePauseSlider.verticalCenter
                                }

                                Accessible.ignored: true
                            }
                        }
                    }

                    // Notification audio
                    LabeledItem {
                        text: qsTr('Notification tone')
                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                        AudioFileSelector {
                            filePath: audioSettings.notificationTone
                            isPlaying: ViewHelper.isPlayingNotificationTone
                            anchors {
                                left: parent.left
                                right: parent.right
                            }

                            onFileSelected: newFilePath => audioSettings.notificationTone = newFilePath
                            onStartTestPlay: () => ViewHelper.testPlayNotificationTone(notificationToneVolumeSlider.value / 100.0)
                            onStopTestPlay: () => ViewHelper.stopTestPlayNotificationTone()
                        }
                    }

                    LabeledItem {
                        text: qsTr('Notification tone volume')
                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                        VolumeSlider {
                            id: notificationToneVolumeSlider
                            anchors {
                                left: parent.left
                                right: parent.right
                            }

                            onMoved: () => ViewHelper.testPlayNotificationTone(notificationToneVolumeSlider.value / 100.0)
                        }
                    }
                }
            }

            Column {
                id: thirdColumn
                spacing: firstColumn.spacing
                leftPadding: 30
                anchors {
                    top: secondColumn.bottom
                    topMargin: secondColumn.spacing
                    left: secondColumn.left
                    right: secondColumn.right
                }

                CardList {
                    id: debuggingCard
                    title: qsTr('Debugging')
                    spacing: 20
                    anchors {
                        left: parent?.left
                        right: parent?.right
                        margins: 20
                    }

                    Label {
                        id: debugRunLabel
                        text: qsTr('Use this button to start a debug run. The App will restart and then begin to record additional information that can be useful for debugging purposes. During this run, come back here to download the information. A debug run is limited to 5 minutes, after which the App will automatically restart again in normal mode.')
                        visible: !ViewHelper.isDebugRun
                        wrapMode: Label.WordWrap
                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                        Accessible.role: Accessible.StaticText
                        Accessible.name: debugRunLabel.text
                    }

                    Button {
                        id: debugRunbutton
                        visible: !ViewHelper.isDebugRun
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr('Start debug run (restart app)')
                        onClicked: () => {
                            genericSettings.setValue("nextDebugRun", true)
                            SM.restart()
                        }

                        Accessible.role: Accessible.Button
                        Accessible.name: debugRunbutton.text
                        Accessible.focusable: true
                        Accessible.onPressAction: () => debugRunbutton.click()
                    }

                    Button {
                        id: debugInfobutton
                        visible: ViewHelper.isDebugRun
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr('Download debug information')
                        onClicked: () => {
                            ViewHelper.downloadDebugInformation()
                            SM.restart()
                        }

                        Accessible.role: Accessible.Button
                        Accessible.name: debugInfobutton.text
                        Accessible.focusable: true
                        Accessible.onPressAction: () => debugInfobutton.click()
                    }

                    Button {
                        id: contactReloadButton
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr('Reload contacts')
                        onClicked: () => ViewHelper.reloadAddressBook()

                        Accessible.role: Accessible.Button
                        Accessible.name: contactReloadButton.text
                        Accessible.focusable: true
                        Accessible.onPressAction: () => contactReloadButton.click()
                    }
                }
            }
        }
    }

    Component {
        id: preferredIdentityEditWindowComponent
        PreferredIdentityEditWindow {}
    }
}
