pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Window
import QtQuick.Controls.Material
import QtQuick.Controls.impl
import base

BaseWindow {
    id: control
    objectName: "sipTemplateWizard"
    width: 600
    height: 600
    visible: true
    resizable: false
    title: qsTr("Initial configuration")

    minimumWidth: control.width
    minimumHeight: control.height
    maximumWidth: control.width
    maximumHeight: control.height

    function finishWizard() {
        const result = sipTemplateController.createConfig(templateModel.templateId, control.values)

        if (result.error) {
            wizardInstallationLabel.text = qsTr("Error: %1").arg(result.error)
            wizardStatusImage.source = "qrc:/icons/data-error.svg"
            wizardInstallationSaveLabel.visible = false
            wizardInstallItem.visible = false
            wizardFinishButton.enabled = false
        } else {
            configPathLabel.text = result.path
        }
    }

    property var values: ({})
    property list<string> requiredTargets: []

    readonly property bool isValid: {
        for (const target of control.requiredTargets) {
            if (!control.values[target]) {
                return false
            }
        }
        return true
    }

    readonly property SIPTemplateController sipController: SIPTemplateController {
        id: sipTemplateController
    }

    Item {
        state: {
            if (templateModel.templateId) {
                if (configPathLabel.text) {
                    return "STEP_3"
                } else if (templatesModel.hasFields(templateModel.templateId)) {
                    return "STEP_2"
                }
                control.finishWizard()
                return "STEP_3"
            }
            return "STEP_1"
        }
        states: [
            State {
                name: "STEP_1"
                PropertyChanges {
                    templateSelectContainer.visible: true
                }
            },
            State {
                name: "STEP_2"
                PropertyChanges {
                    templateFieldContainer.visible: true
                }
            },
            State {
                name: "STEP_3"
                PropertyChanges {
                    finishedHint.visible: true
                }
            }
        ]
    }

    Item {
        id: templateSelectContainer
        visible: false
        anchors {
            fill: parent
            margins: 20
        }

        Column {
            spacing: 20
            anchors {
                left: parent.left
                right: parent.right
            }

            Image {
                source: "qrc:/icons/preferences-system-network.svg"
                sourceSize.width: 128
                sourceSize.height: 128
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Label {
                text: qsTr("GOnnect cannot find a SIP configuration. To get started, pick one of the templates below and modify the resulting configuration file if required.")
                wrapMode: Label.Wrap
                anchors {
                    left: parent.left
                    right: parent.right
                }
            }

            Label {
                text: qsTr("Please pick:")
                wrapMode: Label.Wrap
                anchors {
                    left: parent.left
                    right: parent.right
                }
            }

            ComboBox {
                id: templateSelectBox
                textRole: "name"
                valueRole: "id"
                model: SIPTemplatesModel {
                    id: templatesModel
                }
                anchors {
                    left: parent.left
                    right: parent.right
                }
            }
        }

        Button {
            text: qsTr("Next")
            highlighted: true
            anchors {
                right: parent.right
                bottom: parent.bottom
            }

            onClicked: () => templateModel.templateId = templateSelectBox.currentValue
        }
    }

    Item {
        id: templateFieldContainer
        visible: false
        anchors {
            fill: parent
            margins: 20
        }

        ListView {
            id: fieldsList
            spacing: 20
            anchors.fill: parent
            model: SIPTemplateModel {
                id: templateModel
            }
            delegate: Column {
                id: delg
                anchors {
                    left: parent?.left
                    right: parent?.right
                }

                required property string name
                required property string description
                required property string preset
                required property string target
                required property list<string> fileSuffixes
                required property var regex
                required property int type
                required property bool required

                Component.onCompleted: () => {
                    if (delg.required) {
                        control.requiredTargets = control.requiredTargets.concat([delg.target])
                    }
                }

                Label {
                    id: nameLabel
                    text: delg.name
                    wrapMode: Label.Wrap
                    font.weight: Font.Medium
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }

                Label {
                    id: descriptionLabel
                    text: delg.description
                    wrapMode: Label.Wrap
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }

                Loader {
                    id: widgetLoader
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                    source: {
                        const base = 'qrc:///qt/qml/base/ui/components/templatefields/'

                        switch (delg.type) {
                            case SIPTemplateField.TemplateFieldType.Invalid:
                                return ""
                            case SIPTemplateField.TemplateFieldType.Text:
                                return base + "TemplateFieldText.qml"
                            case SIPTemplateField.TemplateFieldType.Secret:
                                return base + "TemplateFieldSecret.qml"
                            case SIPTemplateField.TemplateFieldType.File:
                                return base + "TemplateFieldFile.qml"
                        }

                        console.error("Error: Unknown template field type:", delg.type)
                        return ""
                    }

                    onItemChanged: () => {
                        const item = widgetLoader.item
                        if (item && item.hasOwnProperty("text") && delg.preset) {
                            item.text = delg.preset

                            const val = {}
                            val[delg.target] = item.value
                            control.values = Object.assign(control.values, val)
                        }
                    }

                    Binding {
                        when: !!widgetLoader.item?.hasOwnProperty("regex")
                        target: widgetLoader.item
                        property: "regex"
                        value: delg.regex
                    }

                    Binding {
                        when: !!widgetLoader.item?.hasOwnProperty("fileSuffixes")
                        target: widgetLoader.item
                        property: "fileSuffixes"
                        value: delg.fileSuffixes
                    }

                    Connections {
                        target: widgetLoader.item
                        function onValueChanged() {
                            const val = {}
                            val[delg.target] = widgetLoader.item?.value
                            control.values = Object.assign(control.values, val)
                        }
                    }
                }
            }
        }

        Button {
            text: qsTr("Back")
            anchors {
                left: parent.left
                bottom: parent.bottom
            }

            onClicked: () => templateModel.templateId = ""
        }

        Button {
            text: qsTr("Finish")
            highlighted: true
            enabled: control.isValid
            anchors {
                right: parent.right
                bottom: parent.bottom
            }

            onClicked: () => control.finishWizard()
        }
    }

    Item {
        id: finishedHint
        visible: false
        anchors {
            fill: parent
            margins: 20
        }

        Column {
            spacing: 20
            anchors {
                left: parent.left
                right: parent.right
            }

            Image {
                id: wizardStatusImage
                source: "qrc:/icons/data-success.svg"
                sourceSize.width: 128
                sourceSize.height: 128
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Label {
                id: wizardInstallationLabel
                text: qsTr("We have created a configuration file for you. Please check if any changes are required to meet your needs and restart GOnnect to activate them.")
                wrapMode: Label.Wrap
                anchors {
                    left: parent.left
                    right: parent.right
                }
            }

            Label {
                id: wizardInstallationSaveLabel
                text: qsTr("The configuration has been saved to:")
                wrapMode: Label.Wrap
                anchors {
                    left: parent.left
                    right: parent.right
                }
            }

            Item {
                id: wizardInstallItem
                implicitHeight: configPathLabel.implicitHeight
                anchors.horizontalCenter: parent.horizontalCenter
                width: configPathLabel.width + clipboardButton.width

                Label {
                    id: configPathLabel
                    font.family: "Courier"
                }

                ClipboardButton {
                    id: clipboardButton
                    text: configPathLabel.text
                    anchors {
                        left: configPathLabel.right
                        leftMargin: 20
                        verticalCenter: parent.verticalCenter
                    }
                }
            }
        }

        Button {
            id: wizardFinishButton
            text: qsTr("Finish")
            highlighted: true
            anchors {
                right: parent.right
                bottom: parent.bottom
            }

            onClicked: () => SM.restart()
        }
    }
}
