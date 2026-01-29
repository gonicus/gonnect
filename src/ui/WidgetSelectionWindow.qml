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
    visible: true
    resizable: false
    showMinimizeButton: false
    showMaximizeButton: false

    minimumWidth: control.width
    maximumWidth: control.width

    minimumHeight: control.dynamicHeight
    maximumHeight: control.dynamicHeight

    property int currentHeight: widgetOptions.implicitHeight + control.windowHeaderPadding
    property int maxHeight: 700 + control.windowHeaderPadding
    property int dynamicHeight: control.currentHeight > control.maxHeight
                            ? control.maxHeight
                            : control.currentHeight

    required property var widgetRoot

    readonly property LoggingCategory lc: LoggingCategory {
        id: category
        name: "gonnect.qml.WidgetSelectionWindow"
        defaultLogLevel: LoggingCategory.Warning
    }

    readonly property Connections editModeConnections: Connections {
        target: SM
        function onUiEditModeChanged() {
            if (!SM.uiEditMode) {
                control.close()
            }
        }
    }

    readonly property Connections windowConnections: Connections {
        target: control
        function onClosing() {
            SM.uiHasActiveEditDialog = false
        }
    }

    CommonWidgets {
        id: widgets
    }

    property int selection: -1

    Flickable {
        id: widgetFlickable
        clip: true
        contentHeight: widgetOptions.implicitHeight
        anchors {
            fill: parent
        }

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            width: 10
        }

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
                    control.selection = currentIndex

                    widgetSettingsModel.clear()

                    switch (currentIndex) {
                        case CommonWidgets.Type.WebView:
                            const newSettings = [
                                { name: qsTr("Title"), setting: "headerTitle", checkable: 0 },
                                { name: qsTr("URL"), setting: "lightModeUrl", checkable: 0 },
                                { name: qsTr("URL (dark mode)"), setting: "darkModeUrl", checkable: 0 },
                                { name: qsTr("Accept all certificates"), setting: "acceptAllCerts", checkable: 1 }
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

                signal settingsFinished

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
                        Layout.bottomMargin: 10

                        required property int index

                        property string value

                        Component {
                            id: delgInputComp

                            TextField {
                                id: delgInput
                                text: ""

                                Connections {
                                    target: widgetSettings
                                    function onSettingsFinished() {
                                        widgetSettingsDelegate.value = delgInput.text
                                    }
                                }
                            }
                        }

                        Component {
                            id: delgCheckComp

                            CheckBox {
                                id: delgCheck

                                Connections {
                                    target: widgetSettings
                                    function onSettingsFinished() {
                                        widgetSettingsDelegate.value = delgCheck.checked.toString()
                                    }
                                }
                            }
                        }

                        Label {
                            id: delgLabel
                            text: widgetSettingsModel.count > 0
                                  ? widgetSettingsModel.get(widgetSettingsDelegate.index).name
                                  : ""
                        }

                        Loader {
                            id: delgLoader
                            Layout.fillWidth: !delgLoader.isCheckable
                            sourceComponent: delgLoader.isCheckable
                                             ? delgCheckComp
                                             : delgInputComp

                            property bool isCheckable: widgetSettingsModel.count > 0
                                                       ? widgetSettingsModel.get(widgetSettingsDelegate.index).checkable
                                                       : false
                        }
                    }
                }
            }

            RowLayout {
                id: widgetButtons
                spacing: 10
                Layout.preferredHeight: 90
                Layout.fillWidth: true
                layoutDirection: Qt.RightToLeft
                Layout.alignment: Qt.AlignRight

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
                        const selection = control.selection

                        const widgetProperties = {
                            widgetId: control.widgetRoot.pageId + id,
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
                                widgetSettings.settingsFinished()
                                for (let i = 0; i < additionalSettings; i++) {
                                    const key = widgetSettingsModel.get(i).setting
                                    const value = widgetSettingsInput.itemAt(i).value
                                    if (key) {
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
