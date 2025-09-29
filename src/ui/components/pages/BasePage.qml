pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import base

Item {
    id: control

    required property string pageId
    required property string name
    required property string icon

    property bool editMode: true
    Connections {
        target: SM
        function onUiEditModeChanged() {
            editMode = SM.uiEditMode
        }
    }

    property alias gridWidth: snapGrid.width
    property alias gridHeight: snapGrid.height
    property alias density: snapGrid.cellSize

    signal gridResized()

    property alias model: widgetModel
    WidgetModel {
        id: widgetModel

        pageId: control.pageId
    }

    property alias writer: pageWriter
    PageWriter {
        id: pageWriter

        pageId: control.pageId
        name: control.name
        icon: control.icon
        model: control.model
    }

    Component {
        id: widgetSelectionWindowComponent
        WidgetSelectionWindow {
            pageRoot: control
        }
    }

    CommonWidgets {
        id: widgets
    }

    Rectangle {
        id: snapGrid
        visible: control.editMode
        anchors.fill: parent
        color: "transparent"

        property int cellSize: 15
        property int dotRadius: 1

        Canvas {
            id: dotGrid
            anchors.fill: parent

            onPaint: {
                let ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.fillStyle = "gray"

                for (let x = 0; x < width; x += snapGrid.cellSize) {
                    for (let y = 0; y < height; y += snapGrid.cellSize) {
                        ctx.beginPath()
                        ctx.arc(x, y, snapGrid.dotRadius, 0, 2*Math.PI)
                        ctx.fill()
                    }
                }
            }

            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
        }

        onWidthChanged: () => {
            if (snapGrid.width <= 0) {
                return
            }

            control.gridResized()
        }
        onHeightChanged: () => {
            if (snapGrid.height <= 0) {
                return
            }

            control.gridResized()
        }
    }

    Rectangle {
        id: pageEdit
        color: Theme.backgroundColor
        height: 20
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left

        property int iconSize: 20
        property int indicatorPadding: 12

        Row {
            id: pageButtons
            spacing: 10
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.leftMargin: pageEdit.indicatorPadding
            anchors.rightMargin: pageEdit.indicatorPadding

            Rectangle {
                id: addIndicator
                visible: control.editMode
                width: addIcon.implicitWidth
                height: pageEdit.iconSize
                color: "transparent"

                IconLabel {
                    id: addIcon
                    anchors.centerIn: parent
                    color: Theme.secondaryTextColor
                    text: qsTr("Add element")
                    icon {
                        source: Icons.listAdd
                        width: pageEdit.iconSize
                        height: pageEdit.iconSize
                    }
                }

                MouseArea {
                    id: addControl
                    parent: addIndicator
                    anchors.fill: parent

                    onClicked: {
                        addSelection.open()
                    }
                }
            }

            Rectangle {
                id: saveIndicator
                visible: control.editMode
                width: saveIcon.implicitWidth
                height: pageEdit.iconSize
                color: "transparent"

                IconLabel {
                    id: saveIcon
                    anchors.centerIn: parent
                    color: Theme.secondaryTextColor
                    text: qsTr("Save")
                    icon {
                        source: Icons.documentSave
                        width: pageEdit.iconSize
                        height: pageEdit.iconSize
                    }
                }

                MouseArea {
                    id: saveControl
                    parent: saveIndicator
                    anchors.fill: parent

                    onClicked: {
                        SM.setSaveDynamicUi(true)
                    }
                }
            }
        }
    }

    Popup {
        id: addSelection
        width: 400
        height: 400
        background: Rectangle {
            radius: 12
            color: Theme.backgroundHeader
        }
        anchors.centerIn: parent
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        property string name: ""
        property int selection: -1

        Column {
            anchors.centerIn: parent
            spacing: 10

            ComboBox {
                id: widgetSelection
                width: addSelection.width / 2
                currentIndex: -1
                model: ListModel {
                    id: widgetEntries
                    ListElement { name: "DateEvents" }
                    ListElement { name: "Favorites" }
                    ListElement { name: "History" }
                    ListElement { name: "Example" }
                }

                onCurrentIndexChanged: {
                    addSelection.name = widgetEntries.get(currentIndex).name
                    addSelection.selection = currentIndex
                }
            }

            Button {
                id: widgetConfirm
                width: addSelection.width / 2
                text: qsTr("Add widget")

                onPressed: {
                    const name = addSelection.name
                    const selection = addSelection.selection

                    const widgetProperties = {
                        name: name.toLowerCase(),
                        type: selection
                    }

                    let widget
                    switch (selection) {
                        case CommonWidgets.Type.DateEvents:
                            widget = widgets.dateEvents.createObject(control, widgetProperties)
                            break
                        case CommonWidgets.Type.Favorites:
                            widget = widgets.favorites.createObject(control, widgetProperties)
                            break
                        case CommonWidgets.Type.History:
                            widget = widgets.history.createObject(control, widgetProperties)
                            break
                        case CommonWidgets.Type.Example:
                            widget = widgets.example.createObject(control, widgetProperties)
                            break
                        default:
                            widget = null
                            console.log("Widget type unknown", selection)
                    }

                    if (widget === null) {
                        console.log("Could not create widget component")
                    } else {
                        widgetModel.add(widget)
                    }

                    addSelection.close()
                }
            }
        }
    }
}
