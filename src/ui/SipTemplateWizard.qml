pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Window
import QtQuick.Controls.Material
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

    readonly property LoggingCategory lc: LoggingCategory {
        id: category
        name: "gonnect.qml.SipTemplateWizard"
        defaultLogLevel: LoggingCategory.Warning
    }

    function finishWizard() {
        const result = sipTemplateController.createConfig(templateModel.templateId, control.values)

        if (result.error) {
            wizardInstallationLabel.text = qsTr("Error: %1").arg(result.error)
            wizardStatusImage.source = Icons.dataError
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
                source: Icons.preferencesSystemNetwork
                sourceSize.width: 128
                sourceSize.height: 128
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Label {
                id: templateHeaderLabel
                text: qsTr("GOnnect cannot find a SIP configuration. To get started, pick one of the templates below and modify the resulting configuration file if required.")
                wrapMode: Label.Wrap
                anchors {
                    left: parent.left
                    right: parent.right
                }

                Accessible.role: Accessible.StaticText
                Accessible.name: qsTr("SIP wizard notification")
                Accessible.description: templateHeaderLabel.text
            }

            Label {
                text: qsTr("Please pick:")
                wrapMode: Label.Wrap
                anchors {
                    left: parent.left
                    right: parent.right
                }

                Accessible.role: Accessible.StaticText
                Accessible.name: qsTr("SIP template selection label")
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

                Accessible.role: Accessible.ComboBox
                Accessible.name: qsTr("Select SIP template")
                Accessible.description: qsTr("Select the SIP template to be used")

                delegate: ItemDelegate {
                    id: templateSelectBoxDelg
                    width: parent.width
                    text: templateSelectBoxDelg.name

                    Accessible.role: Accessible.ListItem
                    Accessible.name: templateSelectBoxDelg.name
                    Accessible.description: qsTr("Currently selected SIP template")
                    Accessible.focusable: true

                    required property string name
                }
            }
        }

        Button {
            id: templateSetupNext
            text: qsTr("Next")
            highlighted: true
            anchors {
                right: parent.right
                bottom: parent.bottom
            }

            onClicked: () => templateModel.templateId = templateSelectBox.currentValue

            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Continue setup")
            Accessible.description: qsTr("Confirmation button to continue the setup")
            Accessible.focusable: true
            Accessible.onPressAction: () => templateSetupNext.clicked()
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

            Accessible.role: Accessible.List
            Accessible.name: qsTr("Template field list")
            Accessible.description: qsTr("List of all the available SIP template options")

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

                Accessible.role: Accessible.ListItem
                Accessible.name: qsTr("SIP template option")
                Accessible.description: qsTr("Currently selected SIP template option")
                Accessible.focusable: true

                Label {
                    id: nameLabel
                    text: delg.name
                    wrapMode: Label.Wrap
                    font.weight: Font.Medium
                    anchors {
                        left: parent.left
                        right: parent.right
                    }

                    Accessible.role: Accessible.StaticText
                    Accessible.name: delg.name
                    Accessible.description: qsTr("Display name of the SIP template option")
                }

                Label {
                    id: descriptionLabel
                    text: delg.description
                    wrapMode: Label.Wrap
                    anchors {
                        left: parent.left
                        right: parent.right
                    }

                    Accessible.role: Accessible.StaticText
                    Accessible.name: delg.description
                    Accessible.description: qsTr("Description of the SIP template option")
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
                            case SIPTemplateField.TemplateFieldType.File:
                                return base + "TemplateFieldFile.qml"
                        }

                        console.error(category, "unknown template field type:", delg.type)
                        return ""
                    }

                    onItemChanged: () => {
                        const item = widgetLoader.item
                        if (item && item.hasOwnProperty("text") && delg.preset) {
                            item.text = delg.preset

                            const val = {}
                            val[delg.target] = item.value
                            control.values = Object.assign({}, control.values, val)
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
                            control.values = Object.assign({}, control.values, val)
                        }
                    }
                }
            }
        }

        Button {
            id: templateBack
            text: qsTr("Back")
            anchors {
                left: parent.left
                bottom: parent.bottom
            }

            onClicked: () => templateModel.templateId = ""

            Accessible.role: Accessible.Button
            Accessible.name: templateBack.text
            Accessible.description: qsTr("Back button to return to the template selection menu")
            Accessible.focusable: true
            Accessible.onPressAction: () => templateBack.clicked()
        }

        Button {
            id: templateFinish
            text: qsTr("Finish")
            highlighted: true
            enabled: control.isValid
            anchors {
                right: parent.right
                bottom: parent.bottom
            }

            onClicked: () => control.finishWizard()

            Accessible.role: Accessible.Button
            Accessible.name: templateFinish.text
            Accessible.description: qsTr("Confirmation button to apply the changes to the SIP template")
            Accessible.focusable: true
            Accessible.onPressAction: () => templateFinish.clicked()
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
                source: Icons.dataSuccess
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

                Accessible.role: Accessible.StaticText
                Accessible.name: qsTr("Successful configuration file creation")
                Accessible.description: wizardInstallationLabel.text
            }

            Label {
                id: wizardInstallationSaveLabel
                text: qsTr("The configuration has been saved to: ")
                wrapMode: Label.Wrap
                anchors {
                    left: parent.left
                    right: parent.right
                }

                Accessible.role: Accessible.StaticText
                Accessible.name: qsTr("Configuration file location")
                Accessible.description: wizardInstallationSaveLabel.text + configPathLabel.text
            }

            Item {
                id: wizardInstallItem
                implicitHeight: configPathLabel.implicitHeight
                anchors.horizontalCenter: parent.horizontalCenter
                width: configPathLabel.width + clipboardButton.width

                Label {
                    id: configPathLabel
                    font.family: "Courier"

                    Accessible.role: Accessible.StaticText
                    Accessible.name: qsTr("Configuration file location")
                    Accessible.description: configPathLabel.text
                }

                ClipboardButton {
                    id: clipboardButton
                    text: configPathLabel.text
                    anchors {
                        left: configPathLabel.right
                        leftMargin: 20
                        verticalCenter: parent.verticalCenter
                    }

                    Accessible.role: Accessible.Button
                    Accessible.name: qsTr("Copy to clipboard")
                    Accessible.description: qsTr("Copy the full path of the configuration file to the clipboard")
                    Accessible.focusable: true
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

            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Finish wizard")
            Accessible.description: qsTr("Finish the SIP configuration wizard")
            Accessible.focusable: true
            Accessible.onPressAction: () => wizardFinishButton.clicked()
        }
    }
}
