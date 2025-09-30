pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Window
import QtQuick.Controls.Material
import base

BaseWindow {
    id: control
    objectName: "widgetSelectionWindow"
    title: qsTr("Add widget")
    width: 600
    height: 380
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

    Column {
        id: widgetOptions
        spacing: 10
        anchors {
            fill: parent
            margins: 20
        }

        Label {
            id: titleLabel
            text: qsTr("Widget")
        }

        ComboBox {
            id: widgetSelection
            width: parent.width / 2
            currentIndex: -1
            model: ListModel {
                id: widgetEntries
                ListElement { name: "DateEvents" }
                ListElement { name: "Favorites" }
                ListElement { name: "History" }
                ListElement { name: "Example" }
            }

            // TODO: Delegate with both widget icon/image and text

            onCurrentIndexChanged: {
                control.name = widgetEntries.get(currentIndex).name
                control.selection = currentIndex
            }
        }

        Row {
            id: widgetButtons
            spacing: 10
            layoutDirection: Qt.RightToLeft
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }

            Button {
                id: widgetCancel
                width: parent.width / 4
                text: qsTr("Cancel")

                onPressed: control.close()
            }

            Button {
                id: widgetConfirm
                width: parent.width / 4
                text: qsTr("Add")

                onPressed: {
                    const name = control.name
                    const selection = control.selection

                    const widgetProperties = {
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
