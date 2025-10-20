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

    required property var pageRoot

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
                ListElement {
                    name: "Example";
                    description: qsTr("An example widget, ha ha")
                }
            }

            delegate: ItemDelegate {
                width: parent.width
                contentItem: RowLayout {
                    spacing: 10
                    IconLabel {
                        icon {
                            source: Icons.userHome
                            height: 96
                            width: 96
                        }
                    }
                    Label {
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
                    icon {
                        source: Icons.userHome
                        height: 96
                        width: 96
                    }
                }
                Label {
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
                        widgetId: pageRoot.pageId+id,
                        name: name.toLowerCase(),
                        type: selection
                    }

                    let widget
                    switch (selection) {
                        case CommonWidgets.Type.DateEvents:
                            widget = widgets.dateEvents.createObject(pageRoot,
                                                                     widgetProperties)
                            break
                        case CommonWidgets.Type.Favorites:
                            widget = widgets.favorites.createObject(pageRoot,
                                                                    widgetProperties)
                            break
                        case CommonWidgets.Type.History:
                            widget = widgets.history.createObject(pageRoot,
                                                                  widgetProperties)
                            break
                        case CommonWidgets.Type.Example:
                            widget = widgets.example.createObject(pageRoot,
                                                                  widgetProperties)
                            break
                        default:
                            widget = null
                            console.log("Widget type unknown", selection)
                    }

                    if (widget === null) {
                        console.log("Could not create widget component")
                    } else {
                        pageRoot.model.add(widget)
                    }

                    control.close()
                }
            }
        }
    }
}
