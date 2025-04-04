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

        property alias busyOnBusy: busyOnBusyCheckBox.checked
        property alias inverseAcceptReject: inverseAcceptRejectCheckBox.checked
        property alias headsetHookOff: headsetHookOffCheckBox.checked
        property alias useHeadset: headsetCheckBox.checked
    }

    Settings {
        id: audioSettings
        location: ViewHelper.userConfigPath
        category: "audio" + SIPAudioManager.currentProfile

        property string ringtone
        property alias ringtonePause: ringTonePauseSlider.value
        property alias ringtoneVolume: ringToneVolumeSlider.value
        property alias preferExternalRinger: externalRingerCheckbox.checked
    }

    Flickable {
        id: flickable
        anchors.fill: parent
        contentHeight: flickaableContainer.implicitHeight

        ScrollBar.vertical: ScrollBar { width: 10 }

        Column {
            id: flickaableContainer
            spacing: 30
            topPadding: 20
            bottomPadding: 20
            anchors {
                left: parent.left
                right: parent.right
                leftMargin: 20
                rightMargin: 20
            }

            CardList {
                title: qsTr('Settings')
                spacing: 20
                anchors {
                    left: parent.left
                    right: parent.right
                }

                CheckBox {
                    id: trayIconDark
                    text: qsTr('Use dark tray icon')
                    anchors {
                        left: parent.left
                        right: parent.right
                    }

                    property bool initialized: false

                    Component.onCompleted: () => {
                        trayIconDark.checked = genericSettings.value('trayIconDark', false)
                        trayIconDark.initialized = true
                    }

                    onCheckedChanged: () => {
                        if (trayIconDark.initialized) {
                            genericSettings.setValue('trayIconDark', trayIconDark.checked)
                            ViewHelper.resetTrayIcon()
                        }
                    }
                }

                CheckBox {
                    id: inverseAcceptRejectCheckBox
                    text: qsTr('Inverse Accept / Reject buttons')
                    anchors {
                        left: parent.left
                        right: parent.right

                    }
                }

                CheckBox {
                    id: headsetCheckBox

                    readonly property HeadsetDeviceProxy headsetProxy: ViewHelper.headsetDeviceProxy()

                    text: qsTr('Enable USB headset driver [%1]').arg(headsetProxy.name === "" ? qsTr("not detected") : headsetProxy.name)
                    anchors {
                        left: parent.left
                        right: parent.right

                    }

                    onCheckedChanged: () => {
                        genericSettings.sync()
                    }
                }

                CheckBox {
                    id: headsetHookOffCheckBox
                    text: qsTr('Show dial window on USB headset pick up')
                    enabled: headsetCheckBox.checked
                    anchors {
                        left: parent.left
                        right: parent.right

                    }
                }

                Column {
                    anchors {
                        left: parent.left
                        right: parent.right
                    }

                    Label {
                        text: qsTr('Color scheme')
                        anchors {
                            left: parent.left
                            right: parent.right
                        }
                    }

                    ComboBox {
                        id: darkModeComboBox
                        editable: false
                        textRole: 'displayName'
                        valueRole: 'value'
                        anchors {
                            left: parent.left
                            right: parent.right

                        }
                        model: [
                            {
                                displayName: qsTr("System default"),
                                value: Theme.ThemeVariant.System
                            }, {
                                displayName: qsTr("Light"),
                                value: Theme.ThemeVariant.Light
                            }, {
                                displayName: qsTr("Dark"),
                                value: Theme.ThemeVariant.Dark
                            }
                        ]

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

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr('Reload contacts from LDAP')
                    onClicked: () => ViewHelper.reloadAddressBook()
                }
            }

            CardList {
                title: qsTr('Phoning')
                anchors {
                    left: parent.left
                    right: parent.right
                }

                CheckBox {
                    id: busyOnBusyCheckBox
                    text: qsTr('Signalling busy when a call is active')
                    anchors {
                        left: parent.left
                        right: parent.right

                    }
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
                        }
                    }
                }
            }

            CardList {
                title: qsTr('Rules for telephone number transmission')
                spacing: 20
                anchors {
                    left: parent.left
                    right: parent.right
                }

                Column {
                    anchors {
                        left: parent.left
                        right: parent.right
                    }

                    Label {
                        text: qsTr('Standard preferred identity')
                        anchors {
                            left: parent.left
                            right: parent.right

                        }
                    }

                    ComboBox {
                        id: standardPreferredIdentitySelector
                        editable: false
                        textRole: 'displayName'
                        valueRole: 'id'
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
                            anchors.horizontalCenter: parent.horizontalCenter
                            visible: preferredIdentitiesRepeater.count === 0
                            text: qsTr("No preferred identities yet.")
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
                                }

                                Label {
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
                }
            }

            CardList {
                id: audioSettingsCard
                title: qsTr('Audio settings')
                spacing: 20
                anchors {
                    left: parent.left
                    right: parent.right
                }

                Column {
                    anchors {
                        left: parent.left
                        right: parent.right
                    }

                    Label {
                        text: qsTr('Input device')
                        anchors {
                            left: parent.left
                            right: parent.right

                        }
                    }

                    ComboBox {
                        id: inputAudioSelector
                        editable: false
                        textRole: 'name'
                        valueRole: 'id'
                        anchors {
                            left: parent.left
                            right: parent.right

                        }
                        model: SIPAudioManager.devices.filter(device => device.isInput)

                        function updateSelectedAudioDeviceFromModel() {
                            const deviceId = SIPAudioManager.captureDeviceId
                            const model = inputAudioSelector.model

                            for (let i = 0; i < model.length; ++i) {
                                if (model[i].id === deviceId) {
                                    inputAudioSelector.currentIndex = i
                                    return
                                }
                            }
                            inputAudioSelector.currentIndex = 0
                        }

                        onActivated: () => SIPAudioManager.captureDeviceId = inputAudioSelector.currentValue

                        Component.onCompleted: () => inputAudioSelector.updateSelectedAudioDeviceFromModel()
                        onModelChanged: () => inputAudioSelector.updateSelectedAudioDeviceFromModel()

                        Connections {
                            target: SIPAudioManager
                            function onCaptureDeviceIdChanged() { inputAudioSelector.updateSelectedAudioDeviceFromModel() }
                        }
                    }
                }

                Column {
                    anchors {
                        left: parent.left
                        right: parent.right
                    }

                    Label {
                        text: qsTr('Output device')
                        anchors {
                            left: parent.left
                            right: parent.right

                        }
                    }

                    ComboBox {
                        id: outputAudioSelector
                        editable: false
                        textRole: 'name'
                        valueRole: 'id'
                        anchors {
                            left: parent.left
                            right: parent.right

                        }
                        model: SIPAudioManager.devices.filter(device => device.isOutput)

                        function updateSelectedAudioDeviceFromModel() {
                            const deviceId = SIPAudioManager.playbackDeviceId
                            const model = outputAudioSelector.model

                            for (let i = 0; i < model.length; ++i) {
                                if (model[i].id === deviceId) {
                                    outputAudioSelector.currentIndex = i
                                    return
                                }
                            }
                            outputAudioSelector.currentIndex = 0
                        }

                        onActivated: () => SIPAudioManager.playbackDeviceId = outputAudioSelector.currentValue

                        Component.onCompleted: () => outputAudioSelector.updateSelectedAudioDeviceFromModel()
                        onModelChanged: () => outputAudioSelector.updateSelectedAudioDeviceFromModel()

                        Connections {
                            target: SIPAudioManager
                            function onPlaybackDeviceIdChanged() { outputAudioSelector.updateSelectedAudioDeviceFromModel() }
                        }
                    }
                }

                Column {
                    anchors {
                        left: parent.left
                        right: parent.right
                    }

                    Label {
                        text: qsTr('Output device for ring tone')
                        anchors {
                            left: parent.left
                            right: parent.right

                        }
                    }

                    ComboBox {
                        id: outputRingToneAudioSelector
                        editable: false
                        textRole: 'name'
                        valueRole: 'id'
                        anchors {
                            left: parent.left
                            right: parent.right

                        }
                        model: SIPAudioManager.devices.filter(device => device.isOutput)


                        function updateSelectedAudioDeviceFromModel() {
                            const deviceId = SIPAudioManager.ringDeviceId
                            const model = outputRingToneAudioSelector.model

                            for (let i = 0; i < model.length; ++i) {
                                if (model[i].id === deviceId) {
                                    outputRingToneAudioSelector.currentIndex = i
                                    return
                                }
                            }
                            outputRingToneAudioSelector.currentIndex = 0
                        }

                        onActivated: () => SIPAudioManager.ringDeviceId = outputRingToneAudioSelector.currentValue ?? ""

                        Component.onCompleted: () => outputRingToneAudioSelector.updateSelectedAudioDeviceFromModel()
                        onModelChanged: () => outputRingToneAudioSelector.updateSelectedAudioDeviceFromModel()

                        Connections {
                            target: SIPAudioManager
                            function onRingDeviceIdChanged() { outputRingToneAudioSelector.updateSelectedAudioDeviceFromModel() }
                        }
                    }
                }

                Column {
                    anchors {
                        left: parent.left
                        right: parent.right
                    }

                    Label {
                        text: qsTr('Ring tone')
                        anchors {
                            left: parent.left
                            right: parent.right

                        }
                    }

                    CheckBox {
                        id: externalRingerCheckbox
                        text: qsTr('Prefer USB headset ring sound if available')
                        enabled: headsetCheckBox.checked
                        anchors {
                            left: parent.left
                            right: parent.right

                        }
                    }

                    Rectangle {
                        height: outputRingToneAudioSelector.height
                        radius: 4
                        color: 'transparent'
                        border.width: 1
                        border.color: Theme.borderColor
                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                        Label {
                            text: {
                                const ringToneFilePath = audioSettings.ringtone
                                if (ringToneFilePath) {
                                    if (ringToneFilePath.startsWith("file://")) {
                                        return ringToneFilePath.substring(7)
                                    }
                                    return ringToneFilePath
                                }
                                return qsTr('Default')
                            }
                            color: audioSettings.ringtone ? Theme.primaryTextColor : Theme.secondaryTextColor
                            maximumLineCount: 2
                            wrapMode: Label.Wrap
                            elide: Label.ElideRight
                            anchors {
                                left: parent.left
                                leftMargin: 10
                                right: testPlayRingToneButton.visible
                                       ? testPlayRingToneButton.left
                                       : (resetToDefaultRingToneButton.visible
                                          ? resetToDefaultRingToneButton.left
                                          : pickRingToneButton.left)
                                rightMargin: 10
                                verticalCenter: parent.verticalCenter
                            }
                        }

                        Button {
                            id: testPlayRingToneButton
                            width: resetToDefaultRingToneButton.height
                            icon.source: ViewHelper.isPlayingRingTone ? Icons.mediaPlaybackPause : Icons.mediaPlaybackStart
                            anchors {
                                right: resetToDefaultRingToneButton.visible ? resetToDefaultRingToneButton.left : pickRingToneButton.left
                                rightMargin: 10
                                verticalCenter: parent.verticalCenter
                            }

                            onClicked: () => {
                                if (ViewHelper.isPlayingRingTone) {
                                    ViewHelper.stopTestPlayRingTone()
                                } else {
                                    ViewHelper.testPlayRingTone(ringToneVolumeSlider.value / 100.0)
                                }
                            }
                        }

                        Button {
                            id: resetToDefaultRingToneButton
                            width: resetToDefaultRingToneButton.height
                            icon.source: Icons.editDelete
                            visible: !!audioSettings.ringtone
                            anchors {
                                right: pickRingToneButton.left
                                rightMargin: 10
                                verticalCenter: parent.verticalCenter
                            }

                            onClicked: () => audioSettings.ringtone = ""
                        }

                        Button {
                            id: pickRingToneButton
                            icon.source: Icons.folderOpen
                            width: pickRingToneButton.height
                            anchors {
                                right: parent.right
                                rightMargin: 10
                                verticalCenter: parent.verticalCenter
                            }
                            onClicked: () => ringToneFileDialog.open()
                        }
                    }
                }

                Column {
                    anchors {
                        left: parent.left
                        right: parent.right
                    }

                    Label {
                        text: qsTr('Ring tone volume')
                        anchors {
                            left: parent.left
                            right: parent.right

                        }
                    }

                    Item {
                        height: ringToneVolumeSlider.height
                        anchors {
                            left: parent.left
                            right: parent.right

                        }

                        Slider {
                            id: ringToneVolumeSlider
                            from: 0
                            to: 100
                            stepSize: 1
                            value: 100
                            anchors {
                                left: parent.left
                                right: ringToneVolumeSliderLabel.left
                                rightMargin: 20
                            }

                            onMoved: () => {
                                ViewHelper.testPlayRingTone(ringToneVolumeSlider.value / 100.0)
                            }
                        }

                        Label {
                            id: ringToneVolumeSliderLabel
                            text: qsTr('%1 %').arg(ringToneVolumeSlider.value)
                            horizontalAlignment: Label.AlignRight
                            width: 40
                            anchors {
                                right: parent.right
                                verticalCenter: ringToneVolumeSlider.verticalCenter
                            }
                        }
                    }
                }

                Column {
                    anchors {
                        left: parent.left
                        right: parent.right
                    }

                    Label {
                        text: qsTr('Pause between ring tones [s]')
                        anchors {
                            left: parent.left
                            right: parent.right

                        }
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
                        }

                        Label {
                            id: ringTonePauseValueLabel
                            text: (ringTonePauseSlider.value / 1000).toLocaleString(Qt.locale(), "f", 2) + " s"
                            horizontalAlignment: Label.AlignRight
                            width: 40
                            anchors {
                                right: parent.right
                                verticalCenter: ringTonePauseSlider.verticalCenter
                            }
                        }
                    }
                }
            }

            CardList {
                title: qsTr('Debugging')
                spacing: 20
                anchors {
                    left: parent.left
                    right: parent.right
                }

                Label {
                    text: qsTr('Use this button to start a debug run. The App will restart and then begin to record additional information that can be useful for debugging purposes. During this run, come back here to download the information. A debug run is limited to 5 minutes, after which the App will automatically restart again in normal mode.')
                    visible: !ViewHelper.isDebugRun
                    wrapMode: Label.WordWrap
                    anchors {
                        left: parent.left
                        right: parent.right

                    }
                }

                Button {
                    visible: !ViewHelper.isDebugRun
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr('Start debug run (restart app)')
                    onClicked: () => {
                        genericSettings.setValue("nextDebugRun", true)
                        SM.restart()
                    }
                }

                Button {
                    visible: ViewHelper.isDebugRun
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr('Download debug information')
                    onClicked: () => {
                        ViewHelper.downloadDebugInformation()
                        SM.restart()
                    }
                }
            }
        }
    }

    FileDialog {
        id: ringToneFileDialog
        onAccepted: audioSettings.ringtone = ringToneFileDialog.selectedFile
        nameFilters: ViewHelper.audioFileSelectors()
    }

    Component {
        id: preferredIdentityEditWindowComponent
        PreferredIdentityEditWindow {}
    }
}
