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

    property string name: ""
    property int selection: -1

    Column {
        spacing: 20
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            margins: 20
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

            onCurrentIndexChanged: {
                control.name = widgetEntries.get(currentIndex).name
                control.selection = currentIndex
            }
        }

        Row {
            id: widgetButtons
            spacing: 10
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }

            Button {
                id: widgetConfirm
                width: parent.width / 2
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
                            widget = pageRoot.widgets.dateEvents.createObject(control,
                                                                              widgetProperties)
                            break
                        case CommonWidgets.Type.Favorites:
                            widget = pageRoot.widgets.favorites.createObject(control,
                                                                             widgetProperties)
                            break
                        case CommonWidgets.Type.History:
                            widget = pageRoot.widgets.history.createObject(control,
                                                                           widgetProperties)
                            break
                        case CommonWidgets.Type.Example:
                            widget = pageRoot.widgets.example.createObject(control,
                                                                           widgetProperties)
                            break
                        default:
                            widget = null
                            console.log("Widget type unknown", selection)
                    }

                    if (widget === null) {
                        console.log("Could not create widget component")
                    } else {
                        pageRoot.widgetModel.add(widget)
                    }

                    control.close()
                }
            }

            Button {
                id: widgetCancel
                width: parent.width / 2
                text: qsTr("Cancel")

                onPressed: control.close()
            }
        }
    }
}
