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

    CommonWidgets {
        id: widgets
    }

    property string name: ""
    property int selection: -1

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
                    description: qsTr("A list of upcoming conferences")
                }
                ListElement {
                    name: "Favorites"
                    description: qsTr("A list of your favorite contacts and conferences")
                }
                ListElement {
                    name: "History"
                    description: qsTr("A searchable call/conference history")
                }
            }

            delegate: ItemDelegate {
                width: parent.width
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
                        text: qsTr("<b>%1</b><br>%2").arg(name)
                                                     .arg(description)
                    }
                }

                required property string name
                required property string description
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
                    const id = "-widget_"+UISettings.generateUuid()
                    const name = control.name
                    const selection = control.selection

                    const widgetProperties = {
                        widgetId: widgetRoot.pageId+id,
                        name: name.toLowerCase(),
                        type: selection,
                        page: widgetRoot
                    }

                    let widget
                    switch (selection) {
                        case CommonWidgets.Type.DateEvents:
                            widget = widgets.dateEvents.createObject(widgetRoot.grid,
                                                                     widgetProperties)
                            break
                        case CommonWidgets.Type.Favorites:
                            widget = widgets.favorites.createObject(widgetRoot.grid,
                                                                    widgetProperties)
                            break
                        case CommonWidgets.Type.History:
                            widget = widgets.history.createObject(widgetRoot.grid,
                                                                  widgetProperties)
                            break
                        default:
                            widget = null
                            console.log("Widget type unknown", selection)
                    }

                    if (widget === null) {
                        console.log("Could not create widget component")
                    } else {
                        widgetRoot.resetWidgetElevation()
                        widgetRoot.model.add(widget)

                        SM.setUiDirtyState(true)
                    }

                    control.close()
                }
            }
        }
    }
}
