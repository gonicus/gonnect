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
                text: qsTr("No fitting audio environment could be found. Please select the desired audio devices.")
                font.pixelSize: 16
                wrapMode: Label.Wrap
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

                    onActivated: () => SIPAudioManager.ringDeviceId = outputRingToneAudioSelector.currentValue

                    Component.onCompleted: () => outputRingToneAudioSelector.updateSelectedAudioDeviceFromModel()

                    Connections {
                        target: SIPAudioManager
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
        }
    }
}
