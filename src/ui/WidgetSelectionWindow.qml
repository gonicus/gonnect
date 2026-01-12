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

    Flickable {
        id: widgetFlickable
        anchors {
            fill: parent
            margins: 20
        }
        contentHeight: widgetOptions.implicitHeight
        clip: true

        ColumnLayout {
            id: widgetOptions
            spacing: 5
            width: parent.width
            Layout.fillHeight: true

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
                        name: qsTr("Date Events")
                        description: qsTr("List of upcoming appointments")
                    }
                    ListElement {
                        name: qsTr("Favorites")
                        description: qsTr("Quick dial for your favorite contacts and conferences")
                    }
                    ListElement {
                        name: qsTr("History")
                        description: qsTr("Searchable call and conference history")
                    }
                    ListElement {
                        name: qsTr("Web View")
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
                            text: "<b>" + widgetDelg.name + "</b><br>"
                                  + widgetDelg.description
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
                        text: "<b>" + widgetEntries.get(widgetSelection.currentIndex).name + "</b><br>"
                              + widgetEntries.get(widgetSelection.currentIndex).description
                    }
                }

                onCurrentIndexChanged: {
                    control.name = widgetEntries.get(currentIndex).name
                    control.selection = currentIndex

                    widgetSettingsModel.clear()

                    switch (currentIndex) {
                        case CommonWidgets.Type.WebView:
                            const newSettings = [
                                { name: qsTr("Header title"), setting: "headerTitle" },
                                { name: qsTr("Dark mode URL"), setting: "darkModeUrl" },
                                { name: qsTr("Light mode URL"), setting: "lightModeUrl" },
                                { name: qsTr("Accept all certificates"), setting: "acceptAllCerts" }
                            ]

                            newSettings.forEach(item => widgetSettingsModel.append(item))
                            break
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

                ListModel {
                    id: widgetSettingsModel
                }

                Repeater {
                    id: widgetSettingsInput
                    model: widgetSettingsModel
                    delegate: ColumnLayout {
                        id: widgetSettingsDelegate
                        spacing: 10
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        required property int index

                        property alias value: delgInput.text

                        Label {
                            id: delgLabel
                            text: widgetSettingsModel.count > 0
                                  ? widgetSettingsModel.get(widgetSettingsDelegate.index).name
                                  : ""
                        }

                        TextField {
                            id: delgInput
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            RowLayout {
                id: widgetButtons
                spacing: 10
                Layout.fillWidth: true
                layoutDirection: Qt.RightToLeft
                Layout.alignment: Qt.AlignRight
                Layout.topMargin: 20
                Layout.bottomMargin: 20

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
                            case CommonWidgets.Type.WebView:
                                widget = widgets.webview.createObject(control.widgetRoot.grid, widgetProperties)
                                break
                            default:
                                widget = null
                                console.error(category, `widget type ${selection} unknown`)
                        }

                        if (widget) {
                            // Per-widget settings
                            const additionalSettings = widgetSettingsInput.count
                            const hasCustomSettings = additionalSettings > 0

                            if (hasCustomSettings) {
                                for (let i = 0; i < additionalSettings; i++) {
                                    const key = widgetSettingsModel.get(i).setting
                                    const value = widgetSettingsInput.itemAt(i).value
                                    if (key && value) {
                                        widget.config.set(key, value)
                                    }
                                }
                                widget.additionalSettingsLoaded()
                            }

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
