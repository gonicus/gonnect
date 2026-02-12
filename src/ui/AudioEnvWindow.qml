import QtQuick
import QtQuick.Controls.Material
import base

BaseWindow {
    id: control
    objectName: "configureAudioEnvWindow"
    width: 710
    height: 604
    visible: true
    title: qsTr("Unknown audio environment")
    resizable: false

    minimumWidth: control.width
    minimumHeight: control.height
    maximumWidth: control.width
    maximumHeight: control.height

    Item {
        anchors.fill: parent

        Column {
            spacing: 20
            topPadding: 20
            bottomPadding: 20
            anchors {
                left: parent.left
                right: parent.right
                leftMargin: 20
                rightMargin: 20
            }

            Label {
                id: audioDeviceError
                text: qsTr("No fitting audio environment could be found. Please select the desired audio devices.")
                font.pixelSize: 16
                wrapMode: Label.Wrap
                anchors {
                    left: parent.left
                    right: parent.right
                }

                Accessible.role: Accessible.StaticText
                Accessible.name: qsTr("Audio environment error")
                Accessible.description: audioDeviceError.text
            }

            Column {
                anchors {
                    left: parent.left
                    right: parent.right
                }

                Label {
                    id: inputAudioLabel
                    text: qsTr('Input device')
                    anchors {
                        left: parent.left
                        right: parent.right
                    }

                    Accessible.role: Accessible.StaticText
                    Accessible.name: qsTr("Input device selection header")
                    Accessible.description: qsTr("Header for the input device selection below")
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
                    model: AudioManager.devices.filter(device => device.isInput)

                    Accessible.role: Accessible.ComboBox
                    Accessible.name: qsTr("Input device selection box")
                    Accessible.description: qsTr("Select the input device that should be used")

                    delegate: ItemDelegate {
                        id: inputAudioSelectorDelg
                        width: parent.width
                        text: inputAudioSelectorDelg.name

                        Accessible.role: Accessible.ListItem
                        Accessible.name: inputAudioSelectorDelg.name
                        Accessible.description: qsTr("Currently selected input device")
                        Accessible.focusable: true
                        Accessible.onPressAction: () => inputAudioSelector.setSelectedAudioDevice()

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

                    function setSelectedAudioDevice() {
                        AudioManager.captureDeviceId = inputAudioSelector.currentValue
                    }

                    onActivated: () => inputAudioSelector.setSelectedAudioDevice()

                    Component.onCompleted: () => inputAudioSelector.updateSelectedAudioDeviceFromModel()

                    Connections {
                        target: AudioManager
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

                    Accessible.role: Accessible.StaticText
                    Accessible.name: qsTr("Output device selection header")
                    Accessible.description: qsTr("Header for the output device selection below")
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
                    model: AudioManager.devices.filter(device => device.isOutput)

                    Accessible.role: Accessible.ComboBox
                    Accessible.name: qsTr("Output device selection box")
                    Accessible.description: qsTr("Select the output device that should be used")

                    delegate: ItemDelegate {
                        id: outputAudioSelectorDelg
                        width: parent.width
                        text: outputAudioSelectorDelg.name

                        Accessible.role: Accessible.ListItem
                        Accessible.name: outputAudioSelectorDelg.name
                        Accessible.description: qsTr("Currently selected output device")
                        Accessible.focusable: true
                        Accessible.onPressAction: () => outputAudioSelector.setSelectedAudioDevice()

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

                    function setSelectedAudioDevice() {
                        AudioManager.playbackDeviceId = outputAudioSelector.currentValue
                    }

                    onActivated: () => outputAudioSelector.setSelectedAudioDevice()

                    Component.onCompleted: () => outputAudioSelector.updateSelectedAudioDeviceFromModel()

                    Connections {
                        target: AudioManager
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
                    id: outputRingToneAudioLabel
                    text: qsTr('Output device for ring tone')
                    anchors {
                        left: parent.left
                        right: parent.right
                    }

                    Accessible.role: Accessible.StaticText
                    Accessible.name: qsTr("Ring tone output device")
                    Accessible.description: outputRingToneAudioLabel.text
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
                    model: AudioManager.devices.filter(device => device.isOutput)

                    Accessible.role: Accessible.ComboBox
                    Accessible.name: qsTr("Ring tone output device selection box")
                    Accessible.description: qsTr("Select the output device that should be used for playing the ring tone")

                    delegate: ItemDelegate {
                        id: outputRingToneAudioSelectorDelg
                        width: parent.width
                        text: outputRingToneAudioSelectorDelg.name

                        Accessible.role: Accessible.ListItem
                        Accessible.name: outputRingToneAudioSelectorDelg.name
                        Accessible.description: qsTr("Currently selected ring tone output device")
                        Accessible.focusable: true
                        Accessible.onPressAction: () => outputRingToneAudioSelectorDelg.setSelectedAudioDevice()

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

                    function setSelectedAudioDevice() {
                        AudioManager.ringDeviceId = outputRingToneAudioSelector.currentValue
                    }

                    onActivated: () => outputRingToneAudioSelectorDelg.setSelectedAudioDevice()

                    Component.onCompleted: () => outputRingToneAudioSelector.updateSelectedAudioDeviceFromModel()

                    Connections {
                        target: AudioManager
                        function onRingDeviceIdChanged() { outputRingToneAudioSelector.updateSelectedAudioDeviceFromModel() }
                    }
                }
            }
        }

        Button {
            id: saveButton
            text: qsTr("Ok")
            highlighted: true
            icon.source: Icons.objectSelectSymbolic

            anchors {
                bottom: parent.bottom
                bottomMargin: 5
                right: parent.right
                rightMargin: 10
            }

            onClicked: () => control.close()

            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Close audio environment selection")
            Accessible.description: qsTr("Confirmation button to leave the audio environment selection window")
            Accessible.focusable: true
            Accessible.onPressAction: () => control.close()
        }
    }
}
