pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls.impl
import QtQuick.Controls.Material
import base

BaseWindow {
    id: control
    objectName: "widgetSelectionWindow"
    title: qsTr("Add widget")
    width: 600
    height: 340
    visible: true
    resizable: false
    showMinimizeButton: false
    showMaximizeButton: false

    minimumWidth: control.width
    minimumHeight: control.height
    maximumWidth: control.width
    maximumHeight: control.height

    required property var widgetRoot

    readonly property LoggingCategory lc: LoggingCategory {
        id: category
        name: "gonnect.qml.WidgetSelectionWindow"
        defaultLogLevel: LoggingCategory.Warning
    }

    CommonWidgets {
        id: widgets
    }

    property string name: ""
    property int selection: -1

    property var additionalSettings: ({})

    Flickable {
        anchors.fill: parent
        contentHeight: widgetOptions.implicitHeight

        ColumnLayout {
            id: widgetOptions
            spacing: 5
            anchors {
                fill: parent
                margins: 20
            }

            Label {
                id: titleLabel
                text: qsTr("Widget")
                Layout.alignment: Qt.AlignTop
            }

            ComboBox {
                id: widgetSelection
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop

                model: ListModel {
                    id: widgetEntries
                    ListElement {
                        name: "DateEvents"
                        description: qsTr("List of upcoming appointments")
                    }
                    ListElement {
                        name: "Favorites"
                        description: qsTr("Quick dial for your favorite contacts and conferences")
                    }
                    ListElement {
                        name: "History"
                        description: qsTr("Searchable call and conference history")
                    }
                    ListElement {
                        name: "Webview"
                        description: qsTr("A web-based content display")
                    }
                }

                delegate: ItemDelegate {
                    id: widgetDelg
                    width: parent.width

                    required property string name
                    required property string description

                    contentItem: RowLayout {
                        spacing: 10

                        IconLabel {
                            id: widgetSelecionPreview
                            Layout.preferredWidth: 96
                            Layout.preferredHeight: 96

                            icon {
                                source: Icons.userHome
                                width: widgetSelecionPreview.width
                                height: widgetSelecionPreview.height
                            }
                        }

                        Label {
                            Layout.fillWidth: true

                            textFormat: Text.RichText
                            text: qsTr("<b>%1</b><br>%2").arg(widgetDelg.name)
                                                         .arg(widgetDelg.description)
                        }
                    }
                }

                contentItem: RowLayout {
                    spacing: 10

                    IconLabel {
                        id: widgetChoicePreview
                        Layout.preferredWidth: 96
                        Layout.preferredHeight: 96

                        icon {
                            source: Icons.userHome
                            width: widgetChoicePreview.width
                            height: widgetChoicePreview.height
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        textFormat: Text.RichText
                        text: qsTr("<b>%1</b><br>%2").arg(widgetEntries.get(widgetSelection.currentIndex).name)
                                                     .arg(widgetEntries.get(widgetSelection.currentIndex).description)
                    }
                }

                onCurrentIndexChanged: {
                    control.name = widgetEntries.get(currentIndex).name
                    control.selection = currentIndex

                    control.additionalSettings = ({})

                    switch (currentIndex) {
                        case CommonWidgets.Type.Webview:
                            widgetSettingsInput.model = ["headerTitle", "darkModeUrl", "lightModeUrl", "acceptAllCerts"]
                            break
                        default:
                            widgetSettingsInput.model = []
                    }
                }
            }

            ColumnLayout {
                id: widgetSettings
                spacing: 10
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.topMargin: 20
                Layout.bottomMargin: 20

                Repeater {
                    id: widgetSettingsInput
                    model: []
                    delegate: ColumnLayout {
                        id: widgetSettingsDelegate
                        spacing: 10
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        required property int index
                        required property string modelData

                        Label {
                            id: delgLabel
                            text: widgetSettingsDelegate.modelData
                        }

                        TextField {
                            id: delgInput
                            Layout.fillWidth: true
                            onTextEdited: { // TODO: Maybe use something less aggressive?
                                control.additionalSettings[delgLabel.text] = delgInput.text
                            }
                        }
                    }
                }
            }

            RowLayout {
                id: widgetButtons
                spacing: 10
                Layout.fillWidth: true
                layoutDirection: Qt.RightToLeft
                Layout.alignment: Qt.AlignRight | Qt.AlignBottom

                Button {
                    id: widgetCancel
                    text: qsTr("Cancel")

                    onPressed: control.close()
                }

                Button {
                    id: widgetConfirm
                    icon.source: Icons.listAdd
                    text: qsTr("Add")

                    onPressed: {
                        const id = `-widget_${UISettings.generateUuid()}`
                        const name = control.name
                        const selection = control.selection

                        const widgetProperties = {
                            widgetId: control.widgetRoot.pageId + id,
                            name: name.toLowerCase(),
                            page: control.widgetRoot,
                            gridWidth: Qt.binding(() => control.widgetRoot.gridWidth),
                            gridHeight: Qt.binding(() => control.widgetRoot.gridHeight),
                            gridCellWidth: Qt.binding(() => control.widgetRoot.gridCellWidth),
                            gridCellHeight: Qt.binding(() => control.widgetRoot.gridCellHeight),
                            xGrid: Math.floor(0.25 * control.widgetRoot.gridWidth / control.widgetRoot.gridCellWidth),
                            yGrid: Math.floor(0.25 * control.widgetRoot.gridHeight / control.widgetRoot.gridCellHeight),
                            widthGrid: Math.floor(0.5 * control.widgetRoot.gridWidth / control.widgetRoot.gridCellWidth),
                            heightGrid: Math.floor(0.5 * control.widgetRoot.gridHeight / control.widgetRoot.gridCellHeight),
                        }

                        let widget
                        switch (selection) {
                            case CommonWidgets.Type.DateEvents:
                                widget = widgets.dateEvents.createObject(control.widgetRoot.grid, widgetProperties)
                                break
                            case CommonWidgets.Type.Favorites:
                                widget = widgets.favorites.createObject(control.widgetRoot.grid, widgetProperties)
                                break
                            case CommonWidgets.Type.History:
                                widget = widgets.history.createObject(control.widgetRoot.grid, widgetProperties)
                                break
                            case CommonWidgets.Type.Webview:
                                widget = widgets.webview.createObject(control.widgetRoot.grid, widgetProperties)
                                break
                            default:
                                widget = null
                                console.error(category, `widget type ${selection} unknown`)
                        }

                        if (widget) {
                            // Per-widget settings
                            Object.entries(control.additionalSettings).forEach(([key, value]) => {
                                console.error(key + ": " + value)
                                widget.config.set(key, value)
                            })
                           widget.additionalSettingsLoaded()

                            control.widgetRoot.resetWidgetElevation()
                            control.widgetRoot.model.add(widget)
                        } else {
                            console.error(category, "could not create widget component")
                        }

                        control.close()
                    }
                }
            }
        }
    }
}
