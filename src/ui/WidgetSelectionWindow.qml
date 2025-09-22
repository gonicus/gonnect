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

            Button {
                id: widgetConfirm
                width: parent.width / 2
                text: qsTr("Add")

                onPressed: {
                    const name = control.name
                    const selection = control.selection

                    console.log(name, selection)

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
